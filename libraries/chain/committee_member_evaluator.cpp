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
                auto& index = db().get_index_type<guard_member_index>().indices().get<by_account>();
                auto iter = index.find(op.guard_member_account);
                if (iter != index.end())
                {
                    db().modify(*iter, [](guard_member_object& obj)
                    {
                        obj.formal = true;
                    }
                    );
                    return iter->id;
                }
                const auto& new_del_object = db().create<guard_member_object>([&](guard_member_object& obj) {
                    obj.guard_member_account = op.guard_member_account;
                    obj.vote_id = vote_id;
                    obj.url = op.url;
                });
                //add a new proposal for all miners to approve or disapprove
                processed_transaction _proposed_trx;
                guard_member_create_operation t_op(op);

                //db().get_global_properties()
                //t_op.guard_member_account = op.guard_member_account;
                const proposal_object& proposal = db().create<proposal_object>([&](proposal_object& proposal) {
                    _proposed_trx.operations.emplace_back(t_op);
                    proposal.proposed_transaction = _proposed_trx;
                    proposal.expiration_time = db().head_block_time() + fc::seconds(GUARD_VOTES_EXPIRATION_TIME);
                    proposal.type = vote_id_type::witness;

                    //proposal should only be approved by guard or miners
                    const auto& acc = db().get_index_type<account_index>().indices().get<by_id>();
                    const auto& iter = db().get_index_type<miner_index>().indices().get<by_account>();
                    std::for_each(iter.begin(), iter.end(), [&](const miner_object& a)
                    {
                        proposal.required_account_approvals.insert(acc.find(a.miner_account)->addr);
                    });

                });
                return new_del_object.id;
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result committee_member_update_evaluator::do_evaluate(const committee_member_update_operation& op)
        {
            try {
                FC_ASSERT(db().get(op.committee_member).guard_member_account == op.guard_member_account);
                return void_result();
            } FC_CAPTURE_AND_RETHROW((op))
        }

        void_result committee_member_update_evaluator::do_apply(const committee_member_update_operation& op)
        {
            try {
                database& _db = db();
                _db.modify(
                    _db.get(op.committee_member),
                    [&](guard_member_object& com)
                {
                    if (op.new_url.valid())
                        com.url = *op.new_url;
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
                const auto lock_balances = _db.get_index_type<lockbalance_index>().indices();
                const auto guard_lock_balances = _db.get_index_type<guard_lock_balance_index>().indices();
                const auto all_guard = _db.get_index_type<guard_member_index>().indices();
                const auto all_miner = _db.get_index_type<miner_index>().indices();
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
                auto &proposals = db().get_index_type<proposal_index>().indices().get<by_id>();
                FC_ASSERT(proposals.find(o.pid) == proposals.end(), "No referred proposal is found, must be invalid resign operation.");
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
