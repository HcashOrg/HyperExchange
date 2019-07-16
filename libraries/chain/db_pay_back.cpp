#include <graphene/chain/database.hpp>
#include <graphene/chain/pay_back_object.hpp>
#include <graphene/chain/asset_object.hpp>

namespace graphene {
	namespace chain {
		graphene::chain::address graphene::chain::database::get_contract_or_account_address(const object_id_type& id) const
		{
			address addr;
			switch (id.type())
			{
			case contract_object_type:
				addr = get(contract_id_type(id)).contract_address;
				break;
			case account_object_type:
				addr = get(account_id_type(id)).addr;
				break;
			default:
				FC_ASSERT(false, "${id} is not contract_object_type or account_object_type", ("id", id));
			}
			return addr;
		}
		void graphene::chain::database::adjust_pay_back_balance(object_id_type payback_owner, asset payback_asset, miner_id_type miner_id /*= miner_id_type(0)*/)
		{
			address addr;
			switch (payback_owner.type())
			{
			case contract_object_type:
				addr = get(contract_id_type(payback_owner)).contract_address;
				std::cout<<addr.address_to_string()<<std::endl;
				break;
			case account_object_type:
				addr = get(account_id_type(payback_owner)).addr;
				break;
			default:
				FC_ASSERT(false, "${payback_owner} is not contract_object_type or account_object_type", ("payback_owner", payback_owner));
			}
			return adjust_pay_back_balance(addr, payback_asset, miner_id);
		}
		void database::adjust_pay_back_balance(address payback_owner, asset payback_asset,miner_id_type miner_id) {
			try {
				if (payback_asset.amount == 0) {
					return;                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
				}
				auto& payback_db = get_index_type<payback_index>().indices().get<by_payback_address_miner>();
				auto itr = payback_db.find(boost::make_tuple(payback_owner, miner_id));
				auto& asset_db = get_index_type<asset_index>().indices().get<by_id>();
				auto asset_iter = asset_db.find(payback_asset.asset_id);

				//FC_ASSERT(asset_iter != asset_db.end(), "this asset doesnt exist");
				if (itr == payback_db.end()) {
					FC_ASSERT(payback_asset.amount > 0, "lock balance error");
					create<pay_back_object>([payback_owner, payback_asset, miner_id](pay_back_object& a) {
						a.miner_id = miner_id;
						a.one_owner_balance = payback_asset;
						a.pay_back_owner = payback_owner;
					});
				}
				else {
					
					modify(*itr, [payback_asset](pay_back_object& b) {
						b.one_owner_balance += payback_asset;
					});
				}
			}FC_CAPTURE_AND_RETHROW((payback_owner)(payback_asset))
		}
		std::map<graphene::chain::miner_id_type, graphene::chain::asset> graphene::chain::database::get_pay_back_balance_mid(address payback_owner, std::string symbol_type) const
		{
			try {
				std::map<miner_id_type, asset> results;
				auto payback_db = get_index_type<payback_index>().indices().get<by_payback_address>().equal_range(payback_owner);

				//FC_ASSERT(payback_db.first != payback_db.second);
				for (auto payback_address_iter : boost::make_iterator_range(payback_db.first, payback_db.second)) {
					if (symbol_type == "") {

						if (payback_address_iter.one_owner_balance > asset(0)) {
							results[payback_address_iter.miner_id] = payback_address_iter.one_owner_balance;
						}

					}
					else {
						//
						auto obj = get_asset(symbol_type);
						FC_ASSERT(obj.valid());
						if (payback_address_iter.one_owner_balance.asset_id == obj->get_id() && payback_address_iter.one_owner_balance.amount > 0) {
							results[payback_address_iter.miner_id] = payback_address_iter.one_owner_balance;
						}


					}
				}

				return results;
			}FC_CAPTURE_AND_RETHROW((payback_owner)(symbol_type))
		}

		std::map<string,asset> database::get_pay_back_balance(address payback_owner,std::string symbol_type)const {
			try {
				std::map<string,asset> results;
				auto payback_db = get_index_type<payback_index>().indices().get<by_payback_address>().equal_range(payback_owner);

				FC_ASSERT(payback_db.first != payback_db.second);
				for (auto payback_address_iter : boost::make_iterator_range(payback_db.first, payback_db.second)) {
					if (symbol_type == "") {
						
						if (payback_address_iter.one_owner_balance > asset(0)) {
							auto miner_obj = get(payback_address_iter.miner_id);
							auto miner_acc = get(miner_obj.miner_account);
							results[miner_acc.name] = payback_address_iter.one_owner_balance;
						}
							
					}
					else {
						//
						auto obj = get_asset(symbol_type);
						FC_ASSERT(obj.valid());
						if (payback_address_iter.one_owner_balance.asset_id == obj->get_id() && payback_address_iter.one_owner_balance.amount > 0) {
							auto miner_obj = get(payback_address_iter.miner_id);
							auto miner_acc = get(miner_obj.miner_account);
							results[miner_acc.name] = payback_address_iter.one_owner_balance;
						}
							
						
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