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

#include <graphene/chain/database.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/witness_schedule_object.hpp>
#include <graphene/chain/protocol/asset.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <iostream>
#include <fc/uint128.hpp>

namespace graphene { namespace chain {

using boost::container::flat_set;

miner_id_type database::get_scheduled_miner( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_miners[ current_aslot % GRAPHENE_PRODUCT_PER_ROUND];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = block_interval();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time(head_block_abs_slot * interval);

   const global_property_object& gpo = get_global_properties();

   if (dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag)
   {
	   slot_num += gpo.parameters.maintenance_skip_slots;
   }
     

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if (when < first_slot_time)
   {
	   return 0;
   }
      
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

uint32_t database::miner_participation_rate()const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   return uint64_t(GRAPHENE_100_PERCENT) * dpo.recent_slots_filled.popcount() / 128;
}


void database::update_witness_random_seed(const SecretHashType& new_secret)
{
	try {
		const dynamic_global_property_object& _dgp = get_dynamic_global_properties();
		SecretHashType current_seed;
		if (_dgp.current_random_seed.valid()) 
			current_seed = *(_dgp.current_random_seed);
		else
		{
			current_seed = SecretHashType();
		}
		fc::sha512::encoder enc;
		fc::raw::pack(enc, new_secret);
		fc::raw::pack(enc, current_seed);
		SecretHashType new_seed = fc::ripemd160::hash(enc.result());
		modify(_dgp, [&](dynamic_global_property_object& dgp) {
			dgp.current_random_seed =(string)new_seed;
		});
		
	} FC_CAPTURE_AND_RETHROW((new_secret))
}


void database::reset_current_collected_fee()
{
	_total_collected_fee = 0;
	_total_collected_fees.clear();
}

void database::modify_current_collected_fee(share_type changed_fee)
{
	_total_collected_fee += changed_fee;
}
void database::modify_current_collected_fee(asset changed_fee)
{
	_total_collected_fees[changed_fee.asset_id] += changed_fee.amount;
}

share_type database::get_miner_pay_per_block(uint32_t block_num) {
	std::vector<share_type> reward_list = { 2558000,1811000,1357000,1357000,1357000 };
	uint32_t block_interval = 25228800;//4 * 365* 24* 720
	uint32_t cur_int = block_num / block_interval;
	if (cur_int > 4) {
		return 0;
	}
	else {
		return reward_list[cur_int];
	}
}

asset database::get_fee_from_block(const signed_block& b)
{
	try {
		for (auto trx : b.transactions)
		{
			for (auto op : trx.operations)
			{
				
			}
		}
	}FC_CAPTURE_AND_RETHROW()
}

void database::update_fee_pool()
{
	try {
		for (const auto& iter : _total_collected_fees)
		{
			_total_fees_pool[iter.first] += iter.second;
		}
	}FC_CAPTURE_AND_RETHROW()
}


void database::pay_miner(const miner_id_type& miner_id,asset trxfee)
{
	// find current account lockbalance
	try {
		const global_property_object& gpo = get_global_properties();
		const auto& dgp = get_dynamic_global_properties();

		auto cache_datas = dgp.current_round_lockbalance_cache;
		auto miner_obj = get(miner_id);
		auto miner_acc = get(miner_obj.miner_account);
		//bonus to senators
		auto committee_count = gpo.active_committee_members.size();
		int64_t all_committee_paid = get_miner_pay_per_block(dgp.head_block_number).value *(GRAPHENE_GUARD_PAY_RATIO) / 100;
		int64_t committee_pay = 0;
		int64_t end_value = int64_t(all_committee_paid) / committee_count;
		for (int i = 0; i < committee_count - 1; i++) {
			auto committe_obj = get(get(gpo.active_committee_members.at(i)).guard_member_account);
			adjust_pay_back_balance(committe_obj.addr, asset(end_value, asset_id_type(0)), miner_acc.name);
			committee_pay += end_value;
		}
		auto committe_obj = get(get(gpo.active_committee_members.at(committee_count-1)).guard_member_account);
		adjust_pay_back_balance(committe_obj.addr, asset(all_committee_paid - committee_pay, asset_id_type(0)),miner_acc.name);

		if (cache_datas.count(miner_id) > 0)
		{
			auto& one_data = cache_datas[miner_id];
			boost::multiprecision::uint256_t all_pledge = boost::multiprecision::uint128_t(miner_obj.pledge_weight.hi);
			all_pledge <<= 64;
			all_pledge +=boost::multiprecision::uint128_t(miner_obj.pledge_weight.lo);
			uint64_t all_paid = get_miner_pay_per_block(dgp.head_block_number).value *(GRAPHENE_ALL_MINER_PAY_RATIO)/100 + trxfee.amount.value;
			auto miner_account_obj = get(miner_obj.miner_account);

			uint64_t pledge_pay_amount = all_paid * (GRAPHENE_MINER_PLEDGE_PAY_RATIO - miner_account_obj.options.miner_pledge_pay_back) / GRAPHENE_ALL_MINER_PAY_RATIO;
			uint64_t all_pledge_paid = 0;
			for (auto one_pledge : one_data)
			{
				
				auto price_obj = dgp.current_price_feed.at(one_pledge.lock_asset_id);

				if (one_pledge.lock_asset_id == asset_id_type(0))
				{
					boost::multiprecision::uint256_t cal_end = one_pledge.lock_asset_amount.value;
					boost::multiprecision::uint256_t amount_cal_end = boost::multiprecision::uint256_t(pledge_pay_amount) * cal_end / all_pledge;
					uint64_t end_value = uint64_t(amount_cal_end);
					all_pledge_paid += end_value;

					//todo lock balance contract reward
					if (one_pledge.lock_balance_contract_addr != address())
					{
						continue;
					}
					auto lock_account = get(one_pledge.lock_balance_account);
					adjust_pay_back_balance(lock_account.addr, asset(end_value, asset_id_type(0)),miner_acc.name);
				}
				else if (!price_obj.settlement_price.is_null() && price_obj.settlement_price.quote.asset_id == asset_id_type(0))
				{
					boost::multiprecision::uint256_t cal_middle = boost::multiprecision::uint256_t(one_pledge.lock_asset_amount.value) * boost::multiprecision::uint256_t(price_obj.settlement_price.quote.amount.value);
					boost::multiprecision::uint256_t cal_end = cal_middle / boost::multiprecision::uint256_t(price_obj.settlement_price.base.amount.value);
					//end value

					boost::multiprecision::uint256_t amount_cal_end = boost::multiprecision::uint256_t( pledge_pay_amount) * cal_end / all_pledge;
					uint64_t end_value = uint64_t(amount_cal_end);
					all_pledge_paid += end_value;
					if (one_pledge.lock_balance_contract_addr != address())
					{
						continue;
					}
					auto lock_account = get(one_pledge.lock_balance_account);
					adjust_pay_back_balance(lock_account.addr, asset(end_value, asset_id_type(0)),miner_acc.name);


				}
			}

			adjust_pay_back_balance(miner_account_obj.addr, asset(all_paid - all_pledge_paid, asset_id_type(0)),miner_acc.name);
		}
		else
		{
			auto miner_account_obj = get(miner_obj.miner_account);
			auto all_paid = gpo.parameters.miner_pay_per_block.value;
			all_paid +=trxfee.amount.value;
			adjust_pay_back_balance(miner_account_obj.addr, asset(all_paid, asset_id_type(0)),miner_acc.name);
		}
		auto supply_added = get_miner_pay_per_block(dgp.head_block_number);
		modify(get(asset_id_type()).dynamic_asset_data_id(*this), [supply_added](asset_dynamic_data_object& d) {
			d.current_supply += supply_added;
		});
	} FC_CAPTURE_AND_RETHROW()
}


void database::update_miner_schedule()
{
	const witness_schedule_object& wso = witness_schedule_id_type()(*this);
	const global_property_object& gpo = get_global_properties();
	const auto& dgp = get_dynamic_global_properties();
	
	auto caluate_slot = [&](vector<boost::multiprecision::uint256_t> vec,int count, boost::multiprecision::uint256_t value) {
		boost::multiprecision::uint256_t init = 0;
		for (int i = 0; i < count; ++i)
		{
			init = init + vec[i];  // or: init=binary_op(init,*first) for the binary_op version
			if (value < init)
				return i;
		}
		return count - 1;

	};
	uint32_t block_num = head_block_num();
	try {//todo remove????
		 // if( pending_state->get_head_block_num() < ALP_V0_7_0_FORK_BLOCK_NUM )
		 // return update_active_delegate_list_v1( block_num, pending_state );
		if (block_num % GRAPHENE_PRODUCT_PER_ROUND == 0)
		{
			
			// modify account pledge_weight
			for (auto& w : gpo.active_witnesses)
			{
				
				modify(get(w), [&](miner_object& miner_obj) 
				{
					miner_obj.pledge_weight = 100 * (miner_obj.total_produced + 1);
					for (auto& one_lock_balance : miner_obj.lockbalance_total)
					{
						const auto& asset_obj = one_lock_balance.second.asset_id(*this);
						if (one_lock_balance.second.asset_id == asset_id_type(0))
						{
							if (one_lock_balance.second.amount.value > 0)
							{
								miner_obj.pledge_weight += fc::uint128_t( one_lock_balance.second.amount.value);
							}
							
							continue;
						}
						if(one_lock_balance.second.asset_id != asset_id_type(0)&&asset_obj.current_feed.settlement_price.is_null())
							continue;
						//calculate asset weight
						{
							boost::multiprecision::uint256_t cal_middle = boost::multiprecision::uint256_t(one_lock_balance.second.amount.value) * boost::multiprecision::uint256_t(asset_obj.current_feed.settlement_price.quote.amount.value);
							boost::multiprecision::uint256_t cal_end = cal_middle / boost::multiprecision::uint256_t(asset_obj.current_feed.settlement_price.base.amount.value);
							fc::uint128_t tran_end =fc::uint128_t( boost::multiprecision::uint128_t(cal_end).str());
							miner_obj.pledge_weight += tran_end;
						}
					}
				});
			}


			


			//
			modify(wso, [&](witness_schedule_object& _wso)
			{
				_wso.current_shuffled_miners.clear();
				_wso.current_shuffled_miners.reserve(GRAPHENE_PRODUCT_PER_ROUND);
				vector< boost::multiprecision::uint256_t > temp_witnesses_weight;
				vector<miner_id_type> temp_active_witnesses;
				boost::multiprecision::uint256_t total_weight = 0;
				for (const miner_id_type& w : gpo.active_witnesses)
				{
					const auto& witness_obj = w(*this);
					auto temp_hi = boost::multiprecision::uint256_t(witness_obj.pledge_weight.hi);
					temp_hi <<= 64;
					total_weight += (temp_hi + boost::multiprecision::uint256_t(witness_obj.pledge_weight.lo)) * boost::multiprecision::uint256_t(witness_obj.participation_rate) / 100;
					temp_witnesses_weight.push_back((temp_hi + boost::multiprecision::uint256_t(witness_obj.pledge_weight.lo)) * boost::multiprecision::uint256_t(witness_obj.participation_rate) / 100);
					temp_active_witnesses.push_back(w);
				}
				fc::sha256 rand_seed;
				if (dgp.current_random_seed.valid())
					rand_seed = fc::sha256::hash(*dgp.current_random_seed);
				else
					rand_seed = fc::sha256::hash(SecretHashType());
				for (uint32_t i = 0, x = 0; i < GRAPHENE_PRODUCT_PER_ROUND; ++i)
				{
					boost::multiprecision::uint256_t r(rand_seed._hash[0]);
					for (int x = 1; x < 4; x++) {
						r <<= 64;
						r += rand_seed._hash[x];
					}
					boost::multiprecision::uint256_t j = (r % total_weight);
					int slot = caluate_slot(temp_witnesses_weight, temp_witnesses_weight.size() - i, j);
					_wso.current_shuffled_miners.push_back(temp_active_witnesses[slot]);
					total_weight -= temp_witnesses_weight[slot];
					std::swap(temp_active_witnesses[slot], temp_active_witnesses[temp_active_witnesses.size() - 1 - i ]);
					std::swap(temp_witnesses_weight[slot], temp_witnesses_weight[temp_active_witnesses.size() - 1 - i]);

					rand_seed = fc::sha256::hash(rand_seed);
				}



			});

		}
		auto all_lockbalance = get_index_type<lockbalance_index>().indices();
		const witness_schedule_object& wso_back = witness_schedule_id_type()(*this);
		std::map<miner_id_type, std::vector<lockbalance_object>> temp_lockbalance_cache;
		std::map<asset_id_type, price_feed> temp_lockbalance_price_feed;
		
		for (const lockbalance_object& one_lockbalance : all_lockbalance)
		{
			auto miner_id = std::find(wso_back.current_shuffled_miners.begin(), wso_back.current_shuffled_miners.end(), one_lockbalance.lockto_miner_account);
			if (miner_id != wso_back.current_shuffled_miners.end())
			{
				std::vector<lockbalance_object> temp_locks;
				if (temp_lockbalance_cache.count(*miner_id))
				{
					temp_locks = temp_lockbalance_cache[*miner_id];
				}
				temp_locks.push_back(one_lockbalance);
				temp_lockbalance_cache[*miner_id] = temp_locks;
				if (temp_lockbalance_price_feed.count(one_lockbalance.lock_asset_id) ==0)
				{
					temp_lockbalance_price_feed[one_lockbalance.lock_asset_id] = get(one_lockbalance.lock_asset_id).current_feed;
				}
				

			}
		}

		//current_price_feed
		modify(dgp, [&](dynamic_global_property_object& _dgp)
		{
			_dgp.current_price_feed = temp_lockbalance_price_feed;

		});
		modify(dgp, [&](dynamic_global_property_object& _dgp)
		{
			_dgp.current_round_lockbalance_cache = temp_lockbalance_cache;

		});


		
		
	}	FC_CAPTURE_AND_RETHROW((block_num))

		
}

} }
