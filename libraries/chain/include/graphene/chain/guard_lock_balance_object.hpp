#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>

namespace graphene {
	namespace chain {
		class guard_lock_balance_object;
		class guard_lock_balance_object : public graphene::db::abstract_object<guard_lock_balance_object> {
		public:
			static const uint8_t space_id = protocol_ids;
			static const uint8_t type_id = guard_lock_balance_object_type;
			guard_member_id_type lock_balance_account;
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;

			asset get_guard_lock_balance() const {
				return asset(lock_asset_amount, lock_asset_id);
			}
			
		};
		struct by_guard_lock;
		using guard_lock_balance_multi_index_type =  multi_index_container <
			guard_lock_balance_object,
			indexed_by <
			ordered_unique< tag<by_id>,
			member<object, object_id_type, &object::id>
			>,
			ordered_unique<
			tag<by_guard_lock>,
			composite_key<
			guard_lock_balance_object,
			member<guard_lock_balance_object, guard_member_id_type, &guard_lock_balance_object::lock_balance_account>,
			member<guard_lock_balance_object, asset_id_type, &guard_lock_balance_object::lock_asset_id>
			>,
			composite_key_compare<
			std::less< guard_member_id_type >,
			std::less< asset_id_type >
			>
			>
			>
		> ;
		using  guard_lock_balance_index =  generic_index<guard_lock_balance_object, guard_lock_balance_multi_index_type> ;
	}
}

FC_REFLECT_DERIVED(graphene::chain::guard_lock_balance_object, (graphene::db::object),
					(lock_balance_account)
					(lock_asset_id)
					(lock_asset_amount))