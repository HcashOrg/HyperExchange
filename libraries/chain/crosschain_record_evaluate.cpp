#include <graphene/chain/crosschain_record_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>
namespace graphene {
	namespace chain {
		void_result crosschain_record_evaluate::do_evaluate(const crosschain_record_operation& o) {
			auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
			auto deposit_to_link_trx = deposit_db.find(o.cross_chain_trx.trx_id);
			if (deposit_to_link_trx != deposit_db.end()){
				if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
					FC_ASSERT("deposit transaction exist");
				}
			}
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.cross_chain_trx.asset_symbol))
				return void_result();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			if (!hdl->valid_config())
				return void_result();
			hdl->validate_link_trx(o.cross_chain_trx);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
			FC_ASSERT(tunnel_itr != tunnel_idx.end());
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			FC_ASSERT(asset_itr != asset_idx.end());
			FC_ASSERT(asset_itr->symbol == o.cross_chain_trx.asset_symbol);
			return void_result();
		}
		void_result crosschain_record_evaluate::do_apply(const crosschain_record_operation& o) {
			auto& tunnel_idx =  db().get_index_type<account_binding_index>().indices().get<by_binded_account>();
//			for (auto tunnel_itr : boost::make_iterator_range(tunnel_idx_range.first,tunnel_idx_range.second)){
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			db().adjust_balance(tunnel_itr->owner, asset(asset_itr->amount_from_string(o.cross_chain_trx.amount).amount, o.asset_id));
			db().adjust_deposit_to_link_trx(o.cross_chain_trx);
//			}
			return void_result();
		}
		void crosschain_record_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_evaluate::do_evaluate(const crosschain_withdraw_operation& o) {
					
			database& d = db();
			auto trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>().equal_range(trx_state->_trx->id());
			int i = 0;
			for (auto trx_iter : boost::make_iterator_range(trx_db.first,trx_db.second)){
				i++;
			}
			FC_ASSERT((i == 0), "This Transaction exist");
			//FC_ASSERT(tunnel_itr->bind_account != o.crosschain_account);
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			FC_ASSERT(asset_itr != asset_idx.end());
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.asset_symbol))
				return void_result();
			auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			if (!hdl->valid_config())
				return void_result();
			bool valid_address = hdl->validate_address(o.crosschain_account);
			FC_ASSERT(valid_address, "crosschain address isn`t valid");
			//FC_ASSERT(asset_itr->symbol == o.asset_symbol);
			
			return void_result();
		}
		void_result crosschain_withdraw_evaluate::do_apply(const crosschain_withdraw_operation& o) {
			
                        database& d = db();
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			d.adjust_balance(o.withdraw_account, asset(-(asset_itr->amount_from_string(o.amount).amount), o.asset_id));
			d.adjust_crosschain_transaction(transaction_id_type(), trx_state->_trx->id(), *(trx_state->_trx),uint64_t(operation::tag<crosschain_withdraw_operation>::value), withdraw_without_sign_trx_uncreate);
                        return void_result();
		}

		void crosschain_withdraw_evaluate::pay_fee() {
			return void();
		}

		void_result crosschain_withdraw_result_evaluate::do_evaluate(const crosschain_withdraw_result_operation& o) {
		
			auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
			auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
			auto combine_trx_iter = originaldb.find(boost::make_tuple(o.cross_chain_trx.trx_id, combine_op_number));
			FC_ASSERT(combine_trx_iter != originaldb.end());
			auto & tx_db_objs = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			auto tx_without_sign_iter = tx_db_objs.find(combine_trx_iter->relate_transaction_id);
			FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "user cross chain tx exist error");
			for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
				auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
				FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
			}
			auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
			auto deposit_to_link_trx = deposit_db.find(o.cross_chain_trx.trx_id);
			if (deposit_to_link_trx != deposit_db.end()) {
				FC_ASSERT((deposit_to_link_trx->acquired_transaction_state == acquired_trx_uncreate),"deposit transaction exist");
// 				if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
// 					FC_ASSERT("deposit transaction exist");
// 				}
			}
            auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.cross_chain_trx.asset_symbol))
				return void_result();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			if (!hdl->valid_config())
				return void_result();
			hdl->validate_link_trx(o.cross_chain_trx);
			//auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			//auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.to_account, o.cross_chain_trx.asset_symbol));
// 			auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
// 			auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
// 			auto combine_trx_iter = originaldb.find(boost::make_tuple(o.cross_chain_trx.trx_id, combine_op_number));
// 
// 			FC_ASSERT(combine_trx_iter != originaldb.end());
			//FC_ASSERT(tunnel_itr != tunnel_idx.end());
                        return void_result();

		}
		void_result crosschain_withdraw_result_evaluate::do_apply(const crosschain_withdraw_result_operation& o) {
			
			auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
			auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
			auto combine_trx_iter = originaldb.find(boost::make_tuple(o.cross_chain_trx.trx_id, combine_op_number));
			db().adjust_crosschain_confirm_trx(o.cross_chain_trx);
			db().adjust_crosschain_transaction(combine_trx_iter->relate_transaction_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_result_operation>::value), withdraw_transaction_confirm);
			return void_result();
		}

		void crosschain_withdraw_result_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_without_sign_evaluate::do_evaluate(const crosschain_withdraw_without_sign_operation& o) {
			auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			for (const auto& crosschain_withdraw_trx_id : o.ccw_trx_ids){
				auto crosschain_withdraw_trx = trx_db.find(crosschain_withdraw_trx_id);
				FC_ASSERT(crosschain_withdraw_trx != trx_db.end(), "Source Transaction doesn`t exist");
			}
			auto trx_current_iter = trx_db.find(trx_state->_trx->id());
			FC_ASSERT(trx_current_iter == trx_db.end(), "Crosschain transaction has been created");
            auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.asset_symbol))
				return void_result();
			auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			if (!hdl->valid_config())
				return void_result();
			auto create_trxs = hdl->turn_trxs(o.withdraw_source_trx);
			auto trx_itr_relate = trx_db.find(trx_state->_trx->id());
			FC_ASSERT(trx_itr_relate == trx_db.end(), "Crosschain transaction has been created");
			std::map<std::string, asset> all_balances;
			const auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_symbol>();
			for (auto& one_trx_id : o.ccw_trx_ids)
			{
				auto trx_itr = trx_db.find(one_trx_id);
				FC_ASSERT(trx_itr != trx_db.end());
				FC_ASSERT(trx_itr->real_transaction.operations.size() == 1);
				const auto withdraw_op = trx_itr->real_transaction.operations[0].get<crosschain_withdraw_evaluate::operation_type>();
				
				FC_ASSERT(create_trxs.trxs.count(withdraw_op.crosschain_account) == 1);
				auto one_trx = create_trxs.trxs[withdraw_op.crosschain_account];
				
				FC_ASSERT(one_trx.to_account == withdraw_op.crosschain_account);
				{
					
					const auto asset_itr = asset_idx.find(withdraw_op.asset_symbol);
					FC_ASSERT(asset_itr != asset_idx.end());
					FC_ASSERT(one_trx.asset_symbol == withdraw_op.asset_symbol);
					if (all_balances.count(withdraw_op.crosschain_account) > 0)
					{
						all_balances[withdraw_op.crosschain_account] = all_balances[withdraw_op.crosschain_account] + asset_itr->amount_from_string(withdraw_op.amount);
					}
					else
					{
						all_balances[withdraw_op.crosschain_account] =  asset_itr->amount_from_string(withdraw_op.amount);
					}
					//FC_ASSERT(asset_itr->amount_from_string(one_trx.amount).amount == asset_itr->amount_from_string(withdraw_op.amount).amount);
				}
			
				
				
			}
			FC_ASSERT(all_balances.size() == create_trxs.trxs.size());
			const auto opt_asset = db().get(o.asset_id);
			char temp_fee[1024];
			string format = "%." + fc::variant(opt_asset.precision).as_string() + "f";
			std::sprintf(temp_fee, format.c_str(), create_trxs.fee);
			auto cross_fee = opt_asset.amount_from_string(graphene::utilities::remove_zero_for_str_amount(temp_fee));
			share_type total_amount = 0;
			share_type total_op_amount = 0;
			for (auto& one_balance : all_balances)
			{
				FC_ASSERT(create_trxs.trxs.count(one_balance.first) == 1);
				total_amount += one_balance.second.amount;
			}

			for (auto& trx : create_trxs.trxs)
			{
				total_op_amount += opt_asset.amount_from_string(trx.second.amount).amount;
			}
			FC_ASSERT(total_amount == (total_op_amount + o.crosschain_fee.amount + cross_fee.amount));
			//FC_ASSERT(o.ccw_trx_ids.size() == create_trxs.size());
			
			
			return void_result();
		}
		void_result crosschain_withdraw_without_sign_evaluate::do_apply(const crosschain_withdraw_without_sign_operation& o) {
			for (const auto& one_trx_id : o.ccw_trx_ids)
			{
				db().adjust_crosschain_transaction(one_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_without_sign_operation>::value), withdraw_without_sign_trx_create, o.ccw_trx_ids);
			}
			

			return void_result();
		}
		void crosschain_withdraw_combine_sign_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_evaluate(const crosschain_withdraw_combine_sign_operation& o) {
			auto & tx_db_objs = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			auto trx_iter = tx_db_objs.find(*(o.signed_trx_ids.begin()));
			FC_ASSERT(trx_iter != tx_db_objs.end());
			auto tx_without_sign_iter = tx_db_objs.find(trx_iter->relate_transaction_id);
			auto tx_combine_sign_iter = tx_db_objs.find(trx_state->_trx->id());
			FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "without sign tx exist error");
			FC_ASSERT(tx_combine_sign_iter == tx_db_objs.end(), "combine sign tx has create");
			for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
				auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
				FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
			}
            return void_result();
		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_apply(const crosschain_withdraw_combine_sign_operation& o) {
                        auto &trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			auto trx_iter = trx_db.find(*(o.signed_trx_ids.begin()));
			db().adjust_crosschain_transaction(trx_iter->relate_transaction_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value), withdraw_combine_trx_create);
			
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.asset_symbol))
				return void_result();
			auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			if (!hdl->valid_config())
				return void_result();
			hdl->broadcast_transaction(o.cross_chain_trx);
			return void_result();
		}
		void crosschain_withdraw_without_sign_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_with_sign_evaluate::do_evaluate(const crosschain_withdraw_with_sign_operation& o) {
			//auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			//auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			
			auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			auto crosschain_without_sign_trx_iter = trx_db.find(o.ccw_trx_id);
			FC_ASSERT(crosschain_without_sign_trx_iter != trx_db.end(), "without sign trx doesn`t exist");
			auto trx_itr = trx_db.find(trx_state->_trx->id());
			FC_ASSERT(trx_itr == trx_db.end(), "This sign tx exist");
			vector<crosschain_trx_object> all_signs;
			set<std::string> signs;
			auto sign_range = db().get_index_type<crosschain_trx_index >().indices().get<by_relate_trx_id>().equal_range(o.ccw_trx_id);
			for (auto sign_obj : boost::make_iterator_range(sign_range.first, sign_range.second)) {
				if (sign_obj.trx_state != withdraw_sign_trx) {
					continue;
				}
				std::string temp_sign;
				auto op = sign_obj.real_transaction.operations[0];
				auto sign_op = op.get<crosschain_withdraw_with_sign_operation>();
				std::string sig = sign_op.ccw_trx_signature;
				signs.insert(sig);
			}
// 			db().get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& obj)
// 			{
// 				const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(obj);
// 				if (p.relate_transaction_id == o.ccw_trx_id&&p.trx_state == withdraw_sign_trx) {
// 					all_signs.push_back(p);
// 				}
// 			});
// 			//set<std::string> signs;
// 			for (const auto & signing : all_signs) {
// 				std::string temp_sign;
// 				auto op = signing.real_transaction.operations[0];
// 				auto sign_op = op.get<crosschain_withdraw_with_sign_operation>();
//                                 std::string sig = sign_op.ccw_trx_signature;
// 				signs.insert(sig);
// 			}
			auto sign_iter = signs.find(o.ccw_trx_signature);
			FC_ASSERT(sign_iter == signs.end(), "Guard has sign this transaction");
			//db().get_index_type<crosschain_trx_index>().indices().get<by_trx_relate_type_stata>();
			//auto trx_iter = trx_db.find(boost::make_tuple(o.ccw_trx_id, withdraw_sign_trx));
		
			return void_result();
		}
		void_result crosschain_withdraw_with_sign_evaluate::do_apply(const crosschain_withdraw_with_sign_operation& o) {
			db().adjust_crosschain_transaction(o.ccw_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_with_sign_operation>::value), withdraw_sign_trx);
			return void_result();
		}
		void crosschain_withdraw_with_sign_evaluate::pay_fee() {

		}
	}
}
