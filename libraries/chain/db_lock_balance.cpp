#include <graphene/chain/database.hpp>

#include <graphene/chain/lockbalance_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/balance_object.hpp>
namespace graphene {
	namespace chain {

		asset database::get_lock_balance(string owner, asset_id_type asset_id) const
		{
			auto& index = get_index_type<lockbalance_index>().indices().get<by_lock_account>();
			//auto itr = index.find(boost::make_tuple(owner, asset_id));
			auto itr = index.find(owner);
			if (itr == index.end())
				return asset(0, asset_id);
			return itr->get_lock_balance();
		}

		// 		asset database::get_lock_balance(const account_object& owner, const asset_object& asset_obj) const
		// 		{
		// 			return get_balance(owner.get_id(), asset_obj.get_id());
		// 		}

		// 		asset database::get_lock_balance(const address& addr, const asset_id_type asset_id) const
		// 		{
		// 			auto& bal_idx = get_index_type<balance_index>();
		// 			const auto& by_owner_idx = bal_idx.indices().get<by_owner>();
		// 			//subscribe_to_item(addr);
		// 			auto itr = by_owner_idx.find(boost::make_tuple(addr, asset_id));
		// 			asset result(0, asset_id);
		// 			if (itr != by_owner_idx.end() && itr->owner == addr)
		// 			{
		// 				result += (*itr).balance;
		// 			}
		// 			return result;
		// 		}

		// 		string database::to_pretty_string(const asset& a)const
		// 		{
		// 			return a.asset_id(*this).amount_to_pretty_string(a.amount);
		// 		}

		void database::adjust_lock_balance(account_id_type account, asset delta)
		{
			try {
				if (delta.amount == 0)
					return;
				auto& by_owner_idx = get_index_type<lockbalance_index>().indices().get<by_lock_to_account_asset>();
				auto itr = by_owner_idx.find(boost::make_tuple(account, delta.asset_id));
				//fc::time_point_sec now = head_block_time();
				if (itr == by_owner_idx.end())
				{
					FC_ASSERT(delta.amount > 0, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}",
						("a", account)
						("b", to_pretty_string(asset(0, delta.asset_id)))
						("r", to_pretty_string(-delta)));
					create<lockbalance_object>([account, delta](lockbalance_object& a) {
						a.lockto_account = account;
						a.lock_balance_amount = delta.amount;
						a.lock_balance_id = delta.asset_id;
					});
				}
				else
				{

					if (delta.amount < 0)
						FC_ASSERT(asset(itr->lock_balance_amount, itr->lock_balance_id) >= -delta, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", ("a", account)("b", to_pretty_string(itr->lock_balance_amount))("r", to_pretty_string(-delta)));
					modify(*itr, [delta, account](lockbalance_object& b) {
						b.lock_balance_amount += delta.amount;
					});

				}


			}FC_CAPTURE_AND_RETHROW((account)(delta))
		}

		// 		void database::adjust_lock_balance(account_id_type account, asset delta)
		// 		{
		// 			try {
		// 				if (delta.amount == 0)
		// 					return;
		// 
		// 				auto& index = get_index_type<account_balance_index>().indices().get<by_account_asset>();
		// 				auto itr = index.find(boost::make_tuple(account, delta));
		// 				if (itr == index.end())
		// 				{
		// 					FC_ASSERT(delta.amount > 0, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}",
		// 						("a", account(*this).name)
		// 						("b", to_pretty_string(asset(0, delta.asset_id)))
		// 						("r", to_pretty_string(-delta)));
		// 					create<account_balance_object>([account, &delta](account_balance_object& b) {
		// 						b.owner = account;
		// 						b.asset_type = delta.asset_id;
		// 						b.balance = delta.amount.value;
		// 					});
		// 				}
		// 				else {
		// 					if (delta.amount < 0)
		// 						FC_ASSERT(itr->get_balance() >= -delta, "Insufficient Balance: ${a}'s balance of ${b} is less than required ${r}", ("a", account(*this).name)("b", to_pretty_string(itr->get_balance()))("r", to_pretty_string(-delta)));
		// 					modify(*itr, [delta](account_balance_object& b) {
		// 						b.adjust_balance(delta);
		// 					});
		// 				}
		// 
		// 			} FC_CAPTURE_AND_RETHROW((account)(delta))
		// 		}
		// 		
		// 	}
	}
}