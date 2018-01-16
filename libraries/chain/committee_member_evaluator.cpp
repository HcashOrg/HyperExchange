/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/committee_member_evaluator.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/protocol/fee_schedule.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/transaction_evaluation_state.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/guard_lock_balance_object.hpp>

#include <fc/smart_ref_impl.hpp>

#define GUARD_VOTES_EXPIRATION_TIME 7*24*3600
#define  MINER_VOTES_REVIWE_TIME  24 * 3600
namespace graphene {
    namespace chain {

        void_result guard_member_create_evaluator::do_evaluate(const guard_member_create_operation& op)
        {
            try {
                //FC_ASSERT(db().get(op.guard_member_account).is_lifetime_member());
                 // account cannot be a miner
                auto& iter = db().get_index_type<miner_index>().indices().get<by_account>();
                FC_ASSERT(iter.find(op.guard_member_account) == iter.end(), "account cannot be a miner.");

                auto guards = db().get_index_type<guard_member_index>().indices();
                auto num = 0;
                std::for_each(guards.begin(), guards.end(), [&num](const guard_member_object &g) {
                    if (g.formal) {
                        ++num;
                    }
                });
                FC_ASSERT(num <= db().get_global_properties().parameters.maximum_guard_count, "No more than 15 guards can be created.");
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        object_id_type guard_member_create_evaluator::do_apply(const guard_member_create_operation& op)
        {
            try {
                vote_id_type vote_id;
                db().modify(db().get_global_properties(), [&vote_id](global_property_object& p) {
                    vote_id = get_next_vote_id(p, vote_id_type::committee);
                });
                const auto& new_del_object = db().create<guard_member_object>([&](guard_member_object& obj) {
                    obj.guard_member_account = op.guard_member_account;
                    obj.vote_id = vote_id;
                    obj.url = op.url;
					obj.formal = false;
                });
                return new_del_object.id;
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result guard_member_update_evaluator::do_evaluate(const guard_member_update_operation& op)
        {
            try {
				const auto&  guard_idx = db().get_index_type<guard_member_index>().indices().get<by_account>();
				auto guard_iter = guard_idx.find(op.guard_member_account);
                FC_ASSERT(guard_iter != guard_idx.end());
				FC_ASSERT(db().get(op.guard_member_account).addr == op.owner_addr);
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result guard_member_update_evaluator::do_apply(const guard_member_update_operation& op)
        {
            try {
                database& _db = db();
				_db.modify(
					*_db.get_index_type<guard_member_index>().indices().get<by_account>().find(op.guard_member_account),
                    //_db.get(op.guard_member_account),
                    [&](guard_member_object& com)
                {
                    if (op.new_url.valid())
                        com.url = *op.new_url;
					if (op.formal.valid())
						com.formal = *op.formal;
                });
				auto& guard_obj = *_db.get_index_type<guard_member_index>().indices().get<by_account>().find(op.guard_member_account);
				_db.modify(_db.get(GRAPHENE_GUARD_ACCOUNT), [&](account_object& a)
				{
					if (guard_obj.formal)
					{
						a.active.account_auths[guard_obj.id] = 100;
					}
					else
					{
						a.active.account_auths.erase(guard_obj.id);
					}
				});
				_db.modify(_db.get(GRAPHENE_RELAXED_COMMITTEE_ACCOUNT), [&](account_object& a) {
					a.active = _db.get(GRAPHENE_GUARD_ACCOUNT).active;
				});
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result committee_member_update_global_parameters_evaluator::do_evaluate(const committee_member_update_global_parameters_operation& o)
        {
            try {
                FC_ASSERT(trx_state->_is_proposed_trx);
                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        void_result committee_member_update_global_parameters_evaluator::do_apply(const committee_member_update_global_parameters_operation& o)
        {
            try {
                db().modify(db().get_global_properties(), [&o](global_property_object& p) {
                    p.pending_parameters = o.new_parameters;
                });

                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }


        void_result committee_member_execute_coin_destory_operation_evaluator::do_evaluate(const committee_member_execute_coin_destory_operation& o)
        {
            try {
                FC_ASSERT(trx_state->_is_proposed_trx);

                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }
        void_result committee_member_execute_coin_destory_operation_evaluator::do_apply(const committee_member_execute_coin_destory_operation& o)
        {
            try {
                //执行退币流程
                database& _db = db();
                //执行miner质押退币流程
                const auto& lock_balances = _db.get_index_type<lockbalance_index>().indices();
                const auto& guard_lock_balances = _db.get_index_type<guard_lock_balance_index>().indices();
                const auto& all_guard = _db.get_index_type<guard_member_index>().indices();
                const auto& all_miner = _db.get_index_type<miner_index>().indices();
                share_type loss_money = o.loss_asset.amount;
                share_type miner_need_pay_money = loss_money * o.commitee_member_handle_percent / 100;
                share_type guard_need_pay_money = 0;
                share_type Total_money = 0;
                share_type Total_pay = 0;
                auto asset_obj = _db.get(o.loss_asset.asset_id);
                auto asset_symbol = asset_obj.symbol;
                if (o.commitee_member_handle_percent != 100) {
                    for (auto& one_balance : lock_balances) {
                        if (one_balance.lock_asset_id == o.loss_asset.asset_id) {
                            Total_money += one_balance.lock_asset_amount;
                            //按比例扣除
                        }
                    }
                    double percent = (double)miner_need_pay_money.value / (double)Total_money.value;
                    for (auto& one_balance : lock_balances) {
                        if (one_balance.lock_asset_id == o.loss_asset.asset_id) {
                            share_type pay_amount = (uint64_t)(one_balance.lock_asset_amount.value*percent);
                            _db.modify(one_balance, [&](lockbalance_object& obj) {
                                obj.lock_asset_amount -= pay_amount;
                            });
                            _db.modify(_db.get(one_balance.lockto_miner_account), [&](miner_object& obj) {
                                if (obj.lockbalance_total.count(asset_symbol)) {
                                    obj.lockbalance_total[asset_symbol] -= pay_amount;
                                }

                            });
                            Total_pay += pay_amount;
                            //按比例扣除
                        }
                    }
                }

                if (o.commitee_member_handle_percent != 0)
                {
                    guard_need_pay_money = loss_money - Total_pay;
                    int count = all_guard.size();
                    share_type one_guard_pay_money = guard_need_pay_money.value / count;
                    share_type remaining_amount = guard_need_pay_money - one_guard_pay_money* count;
                    bool first_flag = true;
                    for (auto& temp_lock_balance : guard_lock_balances)
                    {

                        if (temp_lock_balance.lock_asset_id == o.loss_asset.asset_id)
                        {
                            share_type pay_amount;
                            if (first_flag)
                            {
                                pay_amount = one_guard_pay_money + remaining_amount;
                                first_flag = false;
                            }
                            else
                            {
                                pay_amount = one_guard_pay_money;
                            }

                            _db.modify(temp_lock_balance, [&](guard_lock_balance_object& obj) {
                                obj.lock_asset_amount -= pay_amount;

                            });

                            _db.modify(_db.get(temp_lock_balance.lock_balance_account), [&](guard_member_object& obj) {

                                obj.guard_lock_balance[asset_symbol] -= pay_amount;

                            });

                            Total_pay += pay_amount;
                            //按比例扣除
                        }
                    }

                }
                FC_ASSERT(Total_pay == o.loss_asset.amount);

                //执行跨链修改代理流程
                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        void_result chain::guard_member_resign_evaluator::do_evaluate(const guard_member_resign_operation & o)
        {
            try {
                auto &guards = db().get_index_type<guard_member_index>().indices().get<by_id>();
                FC_ASSERT(guards.find(o.guard_member_account) == guards.end(), "No referred guard is found, must be invalid resign operation.");
                auto num = 0;
                std::for_each(guards.begin(), guards.end(), [&num](const guard_member_object &g) {
                    if (g.formal) {
                        ++num;
                    }
                });
                FC_ASSERT(num > db().get_global_properties().parameters.minimum_guard_count, "No enough guards.");
                return void_result();
            } FC_CAPTURE_AND_RETHROW((o))
        }

        object_id_type chain::guard_member_resign_evaluator::do_apply(const guard_member_resign_operation & o)
        {
            try {
                auto &iter = db().get_index_type<guard_member_index>().indices().get<by_account>();
                auto guard = iter.find(o.guard_member_account);
                db().modify(*guard, [](guard_member_object& g) {
                    g.formal = false;
                });
                return object_id_type(o.guard_member_account.space_id, o.guard_member_account.type_id, o.guard_member_account.instance.value);
            } FC_CAPTURE_AND_RETHROW((o))
        }

    }
} // graphene::chain
