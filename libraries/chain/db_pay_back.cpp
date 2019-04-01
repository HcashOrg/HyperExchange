#include <graphene/chain/database.hpp>
#include <graphene/chain/pay_back_object.hpp>
#include <graphene/chain/asset_object.hpp>

namespace graphene {
	namespace chain {
		void database::adjust_pay_back_balance(address payback_owner, asset payback_asset,const string& miner) {
			try {
				if (payback_asset.amount == 0) {
					return;
				}
				auto& payback_db = get_index_type<payback_index>().indices().get<by_payback_address>();
				auto itr = payback_db.find(payback_owner);
				auto& asset_db = get_index_type<asset_index>().indices().get<by_id>();
				auto asset_iter = asset_db.find(payback_asset.asset_id);

				FC_ASSERT(asset_iter != asset_db.end(), "this asset doesnt exist");
				std::string asset_symbol = asset_iter->symbol;
				if (itr == payback_db.end()) {
					FC_ASSERT(payback_asset.amount > 0, "lock balance error");
					create<pay_back_object>([payback_owner, payback_asset, asset_symbol,miner](pay_back_object& a) {
						a.owner_balance[miner] = payback_asset;
						a.pay_back_owner = payback_owner;
					});
				}
				else {
					if (payback_asset.amount < 0) {
						auto& ba=itr->owner_balance.at(miner);
						FC_ASSERT((ba >= -payback_asset), "balance is not enough");
						if (ba== -payback_asset)
						{
							if (itr->owner_balance.size()>1)
							{
								modify(*itr, [payback_owner, miner](pay_back_object& b) {
									b.owner_balance.erase(miner);
								});
							}
							else
							{
								remove(*itr);
							}
							return;
						}
					}
					modify(*itr, [payback_owner, payback_asset, asset_symbol,miner](pay_back_object& b) {
						b.owner_balance[miner] += payback_asset;
					});
				}
			}FC_CAPTURE_AND_RETHROW((payback_owner)(payback_asset))
		}
		std::map<string,asset> database::get_pay_back_balacne(address payback_owner,std::string symbol_type)const {
			try {
				std::map<string,asset> results;
				auto& payback_db = get_index_type<payback_index>().indices().get<by_payback_address>();
				auto payback_address_iter = payback_db.find(payback_owner);
				FC_ASSERT((payback_address_iter != payback_db.end()), "this address has no pay back balance");
				if (symbol_type == "") {
					for (auto itr : payback_address_iter->owner_balance)
					{
						if (itr.second > asset(0))
							results.insert(itr);
					}
				}
				else {
					//
					auto obj = get_asset(symbol_type);
					FC_ASSERT(obj.valid());
					for (auto iter = payback_address_iter->owner_balance.begin(); iter != payback_address_iter->owner_balance.end();++iter)
					{
						if (iter->second.asset_id == obj->get_id() && iter->second.amount > 0)
							results[iter->first] = iter->second;
					}
				}
				return results;
			}FC_CAPTURE_AND_RETHROW((payback_owner)(symbol_type))			
		}

		void database::adjust_bonus_balance(address bonus_owner, asset bonus)
		{
			try {
				if (bonus.asset_id == asset_id_type() || bonus.amount == 0)
					return;
				auto& bonus_db = get_index_type<bonus_index>().indices().get<by_addr>();
				auto itr = bonus_db.find(bonus_owner);
				const auto& asset_obj = get(bonus.asset_id);

				if (itr == bonus_db.end())
				{
					FC_ASSERT(bonus.amount > 0);
					create<bonus_object>([&bonus_owner, &bonus](bonus_object& a) {
						a.owner = bonus_owner;
						a.bonus[bonus.asset_id] += bonus.amount;
					});
				}
				else
				{
					if (itr->bonus.count(bonus.asset_id) == 0)
					{
						FC_ASSERT(bonus.amount >0 );
					}
					else {
						FC_ASSERT(itr->bonus.at(bonus.asset_id) >= -bonus.amount);
					}
					modify(*itr, [&bonus_owner, &bonus](bonus_object& a) {
						a.bonus[bonus.asset_id] += bonus.amount;
					});
				}

			}FC_CAPTURE_AND_RETHROW((bonus_owner)(bonus))
		}

		std::map<string, share_type> database::get_bonus_balance(address owner) const
		{
			std::map<string, share_type> result;
			auto& bonus_db = get_index_type<bonus_index>().indices().get<by_addr>();
			auto itr = bonus_db.find(owner);
			if (itr != bonus_db.end())
			{
				for (const auto& bal : itr->bonus)
				{
					const auto& asset_obj = get(bal.first);
					if (bal.second > 0)
						result[asset_obj.symbol] = bal.second;
				}
			}
			return result;
		}
	}
}