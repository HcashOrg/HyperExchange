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

namespace graphene { namespace chain {

using boost::container::flat_set;

witness_id_type database::get_scheduled_witness( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = witness_schedule_id_type()(*this);
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_witnesses[ current_aslot % GRAPHENE_PRODUCT_PER_ROUND];
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

   if( dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag )
      slot_num += gpo.parameters.maintenance_skip_slots;

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

uint32_t database::witness_participation_rate()const
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


void database::update_witness_schedule()
{
	const witness_schedule_object& wso = witness_schedule_id_type()(*this);
	const global_property_object& gpo = get_global_properties();
	const auto& dgp = get_dynamic_global_properties();
	
	auto caluate_slot = [&](vector<uint64_t> vec,int count,uint64_t value) {
		uint64_t init = 0;
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
		printf("block_num: %d\n",block_num);
		if (block_num % GRAPHENE_PRODUCT_PER_ROUND == 0)
		{
			modify(wso, [&](witness_schedule_object& _wso)
			{
				_wso.current_shuffled_witnesses.clear();
				_wso.current_shuffled_witnesses.reserve(GRAPHENE_PRODUCT_PER_ROUND);
				vector< uint64_t > temp_witnesses_weight;
				vector<witness_id_type> temp_active_witnesses;
				uint64_t total_weight = 0;
				for (const witness_id_type& w : gpo.active_witnesses)
				{
					const auto& witness_obj = w(*this);
					total_weight += witness_obj.pledge_weight * witness_obj.participation_rate / 100;
					temp_witnesses_weight.push_back(witness_obj.pledge_weight * witness_obj.participation_rate / 100);
					temp_active_witnesses.push_back(w);
				}
				fc::sha256 rand_seed;
				if (dgp.current_random_seed.valid())
					rand_seed = fc::sha256::hash(*dgp.current_random_seed);
				else
					rand_seed = fc::sha256::hash(SecretHashType());
				for (uint32_t i = 0, x = 0; i < GRAPHENE_PRODUCT_PER_ROUND; ++i)
				{
					uint64_t r = rand_seed._hash[x];
					uint64_t j = (r % total_weight);
					int slot = caluate_slot(temp_witnesses_weight, temp_witnesses_weight.size() - i, j);
					_wso.current_shuffled_witnesses.push_back(temp_active_witnesses[slot]);
					total_weight -= temp_witnesses_weight[slot];
					std::swap(temp_active_witnesses[slot], temp_active_witnesses[temp_active_witnesses.size() - 1 - i ]);
					std::swap(temp_witnesses_weight[slot], temp_witnesses_weight[temp_active_witnesses.size() - 1 - i]);

					x = (x + 1) & 3;
					if (x == 0)
						rand_seed = fc::sha256::hash(rand_seed);
				}



			});

		}
	}	FC_CAPTURE_AND_RETHROW((block_num))

		
}

} }
