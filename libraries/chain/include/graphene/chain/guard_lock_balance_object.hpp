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
			account_id_type lock_balance_account;
			asset_id_type lock_asset_id;
			share_type lock_asset_amount;

			asset get_lock_balance() const {
				return asset(lock_asset_amount, lock_asset_id);
			}
			
		};
	}
}