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
#pragma once
#include <fc/uint128.hpp>

#include <graphene/chain/protocol/chain_parameters.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/db/object.hpp>
#include <graphene/chain/lockbalance_object.hpp>

namespace graphene { namespace chain {

   /**
    * @class global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are set by committee_members to tune the blockchain parameters.
    */
   class global_property_object : public graphene::db::abstract_object<global_property_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_global_property_object_type;

         chain_parameters           parameters;
         optional<chain_parameters> pending_parameters;
		 bool                       event_need =false;
         uint32_t                           next_available_vote_id = 0;
         vector<guard_member_id_type>   active_committee_members; // updated once per maintenance interval
         flat_set<miner_id_type>          active_witnesses; // updated once per maintenance interval
		 vector<guard_member_id_type> pledge_insufficient_committee_members; // updated once per maintenance interval 
		 std::map<uint32_t, uint32_t> unorder_blocks_match;

         // n.b. witness scheduling is done by witness_schedule object
   };

   /**
    * @class dynamic_global_property_object
    * @brief Maintains global state information (committee_member list, current fees)
    * @ingroup object
    * @ingroup implementation
    *
    * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
    * current values of global blockchain properties.
    */
   class dynamic_global_property_object : public abstract_object<dynamic_global_property_object>
   {
      public:
         static const uint8_t space_id = implementation_ids;
         static const uint8_t type_id  = impl_dynamic_global_property_object_type;

         uint32_t          head_block_number = 0;
         block_id_type     head_block_id;
         time_point_sec    time;
         miner_id_type   current_witness;
         time_point_sec    next_maintenance_time;
         time_point_sec    last_budget_time;
         share_type        miner_budget;
         uint32_t          accounts_registered_this_interval = 0;
		 optional<SecretHashType>      current_random_seed;
		 std::map<miner_id_type,std::vector<lockbalance_object>> current_round_lockbalance_cache;
		 std::map<asset_id_type, price_feed>  current_price_feed;
		 fc::flat_set<miner_id_type>          round_produced_miners;
		 share_type       contract_transfer_fee_rate = 0;

         /**
          *  Every time a block is missed this increases by
          *  RECENTLY_MISSED_COUNT_INCREMENT,
          *  every time a block is found it decreases by
          *  RECENTLY_MISSED_COUNT_DECREMENT.  It is
          *  never less than 0.
          *
          *  If the recently_missed_count hits 2*UNDO_HISTORY then no new blocks may be pushed.
          */
         uint32_t          recently_missed_count = 0;

         /**
          * The current absolute slot number.  Equal to the total
          * number of slots since genesis.  Also equal to the total
          * number of missed slots plus head_block_number.
          */
         uint64_t                current_aslot = 0;

         /**
          * used to compute witness participation.
          */
         fc::uint128_t recent_slots_filled;

         /**
          * dynamic_flags specifies chain state properties that can be
          * expressed in one bit.
          */
         uint32_t dynamic_flags = 0;

         uint32_t last_irreversible_block_num = 0;
		 share_type  bonus_distribute_limit = GRAPHENE_BONUS_DISTRIBUTE_LIMIT;
		 bool referendum_flag = false;
		 time_point_sec next_vote_time;

         enum dynamic_flag_bits
         {
            /**
             * If maintenance_flag is set, then the head block is a
             * maintenance block.  This means
             * get_time_slot(1) - head_block_time() will have a gap
             * due to maintenance duration.
             *
             * This flag answers the question, "Was maintenance
             * performed in the last call to apply_block()?"
             */
            maintenance_flag = 0x01
         };
   };

   class lockbalance_record_object :public abstract_object<lockbalance_record_object>
   {
   public:
	   static const uint8_t space_id = implementation_ids;
	   static const uint8_t type_id = impl_lockbalance_record_object_type;
	   std::map<address, std::map<asset_id_type,share_type>> record_list;
   };
}}

FC_REFLECT_DERIVED(graphene::chain::lockbalance_record_object, (graphene::db::object),(record_list))


FC_REFLECT_DERIVED( graphene::chain::dynamic_global_property_object, (graphene::db::object),
                    (head_block_number)
                    (head_block_id)
                    (time)
                    (current_witness)
                    (next_maintenance_time)
                    (last_budget_time)
                    (miner_budget)
                    (accounts_registered_this_interval)
					(current_random_seed)
                    (recently_missed_count)
                    (current_aslot)
                    (recent_slots_filled)
                    (dynamic_flags)
                    (last_irreversible_block_num)
					(current_round_lockbalance_cache)
					(current_price_feed)
	                (round_produced_miners)
	                (bonus_distribute_limit)
					(contract_transfer_fee_rate)
 	                (referendum_flag)
	                (next_vote_time)
                  )

FC_REFLECT_DERIVED( graphene::chain::global_property_object, (graphene::db::object),
                    (parameters)
                    (pending_parameters)
	                (event_need)
                    (next_available_vote_id)
                    (active_committee_members)
                    (active_witnesses)
	                (unorder_blocks_match)
                  )