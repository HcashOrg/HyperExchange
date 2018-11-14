#include <graphene/chain/coldhot_transfer_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/committee_member_object.hpp>

namespace graphene {
	namespace chain {
		void_result coldhot_transfer_evaluate::do_evaluate(const coldhot_transfer_operation& o) {
			try {
				database& d = db();
				auto & asset_idx = d.get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				FC_ASSERT(asset_itr != asset_idx.end());
				FC_ASSERT(asset_itr->symbol == o.asset_symbol);
				auto float_pos = o.amount.find(".");
				if (float_pos != o.amount.npos)
				{
					FC_ASSERT(o.amount.substr(float_pos + 1).size() <= asset_itr->precision, "amount precision error");
				}
				auto temp_asset = asset_itr->amount_from_string(o.amount);
				FC_ASSERT(temp_asset.amount > 0 ,"transfer amount should exceed zero.");
				auto coldhot_range = d.get_index_type<multisig_account_pair_index>().indices().get<by_chain_type>().equal_range(o.asset_symbol);
				bool withdraw_multi = false;
				bool deposit_multi = false;
				for (auto multiAccountObj : boost::make_iterator_range(coldhot_range.first, coldhot_range.second)) {
					if (o.multi_account_withdraw == multiAccountObj.bind_account_hot || o.multi_account_withdraw == multiAccountObj.bind_account_cold) {
						withdraw_multi = true;
					}
					if (o.multi_account_deposit == multiAccountObj.bind_account_hot || o.multi_account_deposit == multiAccountObj.bind_account_cold) {
						deposit_multi = true;
					}
					if (withdraw_multi && deposit_multi) {
						break;
					}
				}
				FC_ASSERT((withdraw_multi &&deposit_multi), "Cant Transfer account which is not multi account");
				//FC_ASSERT((o.multi_account_deposit != o.multi_account_withdraw), "Cant Transfer account which is accually same account");
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				if (trx_state->_trx == nullptr)
					return void_result();
				auto current_trx_id = trx_state->_trx->id();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(current_trx_id);
				FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "This coldhot Transaction exist");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_transfer_evaluate::do_apply(const coldhot_transfer_operation& o) {
			try {
				database& d = db();
				FC_ASSERT(trx_state->_trx != nullptr);
				d.adjust_coldhot_transaction(transaction_id_type(), trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_operation>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}

		void coldhot_transfer_evaluate::pay_fee(){

		}

		void_result coldhot_transfer_without_sign_evaluate::do_evaluate(const coldhot_transfer_without_sign_operation& o) {
			try {
				database& d = db();
				auto obj = db().get_asset(o.asset_symbol);
				FC_ASSERT(obj.valid());
				//check coldhot transfer trx whether exist
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(o.coldhot_trx_id);
				FC_ASSERT(coldhot_tx_iter != coldhot_tx_dbs.end(), "coldhot original trx doesn`t exist");
				//check without sign trx whether exist
				auto& coldhot_without_sign_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>();
				if (trx_state->_trx == nullptr)
					return void_result();
				auto coldhot_without_sign_tx_iter = coldhot_without_sign_tx_dbs.find(trx_state->_trx->id());
				FC_ASSERT(coldhot_without_sign_tx_iter == coldhot_without_sign_tx_dbs.end(), "coldhot trx without sign has been created");
				
				auto& instance = graphene::crosschain::crosschain_manager::get_instance();
				if (!instance.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto crosschain_handle = instance.get_crosschain_handle(o.asset_symbol);
				if (!crosschain_handle->valid_config())
					return void_result();
				crosschain_trx created_trx;
				if (o.asset_symbol.find("ERC") != o.asset_symbol.npos)
				{

					fc::mutable_variant_object mul_obj;
					auto descrip = obj->options.description;
					auto precision_pos = descrip.find('|');
					int64_t erc_precision = 18;
					if (precision_pos != descrip.npos)
					{
						erc_precision = fc::to_int64(descrip.substr(precision_pos + 1));
					}
					mul_obj.set("precision", erc_precision);
					mul_obj.set("turn_without_eth_sign", o.coldhot_trx_original_chain);
					created_trx = crosschain_handle->turn_trxs(fc::variant_object(mul_obj));
				}
				else if (o.asset_symbol == "ETH")
				{
					created_trx = crosschain_handle->turn_trxs(fc::variant_object("turn_without_eth_sign", o.coldhot_trx_original_chain));
				}
				else {
					created_trx = crosschain_handle->turn_trxs(o.coldhot_trx_original_chain);
				}
				//auto created_trx = crosschain_handle->turn_trxs(o.coldhot_trx_original_chain);
				
				//FC_ASSERT(created_trx.size() == 1);
				//auto & coldhot_trx_db = db().get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				//auto coldhot_original_trx_iter = coldhot_tx_dbs.find(o.coldhot_trx_id);
				//FC_ASSERT(coldhot_original_trx_iter != coldhot_tx_dbs.end());
				FC_ASSERT(coldhot_tx_iter->current_trx.operations.size() == 1);
				for (const auto & op : coldhot_tx_iter->current_trx.operations) {
					/*const auto proposal_update_op = op.get<proposal_create_operation>();
					FC_ASSERT(proposal_update_op.proposed_ops.size() == 1);
					for (const auto & real_op : proposal_update_op.proposed_ops){*/
					const auto coldhot_transfer_op = op.get<coldhot_transfer_operation>();
					auto& multi_account_pair_hot_db = d.get_index_type<multisig_account_pair_index>().indices().get<by_bindhot_chain_type>();
					auto& multi_account_pair_cold_db = d.get_index_type<multisig_account_pair_index>().indices().get<by_bindcold_chain_type>();
					multisig_account_pair_id_type withdraw_multi_id;
					auto hot_iter = multi_account_pair_hot_db.find(boost::make_tuple(coldhot_transfer_op.multi_account_withdraw, coldhot_transfer_op.asset_symbol));
					auto cold_iter = multi_account_pair_cold_db.find(boost::make_tuple(coldhot_transfer_op.multi_account_withdraw, coldhot_transfer_op.asset_symbol));
					if (hot_iter != multi_account_pair_hot_db.end()) {
						withdraw_multi_id = hot_iter->id;
					}
					else if (cold_iter != multi_account_pair_cold_db.end()) {
						withdraw_multi_id = cold_iter->id;
					}
					const auto & multi_range = d.get_index_type<multisig_address_index>().indices().get<by_multisig_account_pair_id>().equal_range(withdraw_multi_id);
					int withdraw_account_count = 0;
					for (auto multi_account : boost::make_iterator_range(multi_range.first, multi_range.second)) {
						++withdraw_account_count;
					}
					FC_ASSERT((withdraw_account_count == o.withdraw_account_count),"withdraw multi account count error");
					FC_ASSERT(created_trx.trxs.count(coldhot_transfer_op.multi_account_deposit) == 1);
					auto one_trx = created_trx.trxs[coldhot_transfer_op.multi_account_deposit];
					FC_ASSERT(one_trx.to_account == coldhot_transfer_op.multi_account_deposit);
					FC_ASSERT(one_trx.from_account == coldhot_transfer_op.multi_account_withdraw);
					const auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_symbol>();
					const auto asset_itr = asset_idx.find(coldhot_transfer_op.asset_symbol);
					FC_ASSERT(asset_itr != asset_idx.end());
					FC_ASSERT(one_trx.asset_symbol == coldhot_transfer_op.asset_symbol);
					FC_ASSERT(asset_itr->amount_from_string(one_trx.amount).amount == asset_itr->amount_from_string(coldhot_transfer_op.amount).amount);
					//}
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result coldhot_transfer_without_sign_evaluate::do_apply(const coldhot_transfer_without_sign_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_coldhot_transaction(o.coldhot_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_without_sign_operation>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}

		void coldhot_transfer_without_sign_evaluate::pay_fee() {

		}

		void_result coldhot_transfer_with_sign_evaluate::do_evaluate(const coldhot_transfer_with_sign_operation& o) {
			try {
				database & d = db();
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_without_sign_tx_iter = coldhot_tx_dbs.find(o.coldhot_trx_id);
				FC_ASSERT(coldhot_without_sign_tx_iter != coldhot_tx_dbs.end(), "coldhot original trx doesn`t exist");
				if (trx_state->_trx == nullptr)
					return void_result();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(trx_state->_trx->id());
				FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "coldhot trx with this sign has been created");

				auto range = d.get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(o.coldhot_trx_id);
				bool isSigned = false;
				for (auto coldhot_sign_tx : boost::make_iterator_range(range.first, range.second)) {
					FC_ASSERT(coldhot_sign_tx.curret_trx_state <= coldhot_sign_trx && coldhot_sign_tx.curret_trx_state  >= coldhot_without_sign_trx_create);
					if (coldhot_sign_tx.curret_trx_state != coldhot_sign_trx) {
						continue;
					}
					FC_ASSERT(coldhot_sign_tx.current_trx.operations.size() == 1);
					auto op = coldhot_sign_tx.current_trx.operations[0];
					auto sign_op = op.get<coldhot_transfer_with_sign_operation>();
					if (sign_op.coldhot_transfer_sign == o.coldhot_transfer_sign) {
						isSigned = true;
						break;
					}
				}
				FC_ASSERT(!isSigned, "Guard has sign this transaction");

				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				auto hd_trxs = hdl->turn_trxs(o.coldhot_trx_original_chain);
				FC_ASSERT(hd_trxs.trxs.size() >= 1);
				auto crosschain_trx = hd_trxs.trxs.begin()->second;  
				vector<multisig_address_object>  senator_pubks = db().get_multi_account_senator(crosschain_trx.from_account, o.asset_symbol);
				FC_ASSERT(senator_pubks.size() > 0);
				auto& acc_idx = db().get_index_type<account_index>().indices().get<by_address>();
				auto acc_itr = acc_idx.find(o.guard_address);
				FC_ASSERT(acc_itr != acc_idx.end());
				int index = 0;
				for (; index < senator_pubks.size(); index++)
				{
					if (senator_pubks[index].guard_account == acc_itr->get_id())
						break;
				}
				FC_ASSERT(index < senator_pubks.size());
				auto multisig_account_obj = db().get(senator_pubks[index].multisig_account_pair_object_id);
				FC_ASSERT(hdl->validate_transaction(senator_pubks[index].new_pubkey_hot, multisig_account_obj.redeemScript_hot, o.coldhot_transfer_sign) || hdl->validate_transaction(senator_pubks[index].new_pubkey_cold, multisig_account_obj.redeemScript_cold, o.coldhot_transfer_sign));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_transfer_with_sign_evaluate::do_apply(const coldhot_transfer_with_sign_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx!= nullptr);
				db().adjust_coldhot_transaction(o.coldhot_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_with_sign_operation>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}

		void coldhot_transfer_with_sign_evaluate::pay_fee() {

		}

		void_result coldhot_transfer_combine_sign_evaluate::do_evaluate(const coldhot_transfer_combine_sign_operation& o) {
			try {
				database & d = db();
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_without_sign_tx_iter = coldhot_tx_dbs.find(o.coldhot_transfer_trx_id);
				FC_ASSERT(coldhot_without_sign_tx_iter != coldhot_tx_dbs.end(), "coldhot without sign trx doesn`t exist");
				//auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				if (trx_state->_trx == nullptr)
					return void_result();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(trx_state->_trx->id());
				FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "coldhot trx combine has been created");
				auto coldhot_relate_original_iter = coldhot_tx_dbs.find(coldhot_without_sign_tx_iter->relate_trx_id);
				FC_ASSERT(coldhot_relate_original_iter != coldhot_tx_dbs.end(), "coldhot transfer trx doesn`t exist");
				FC_ASSERT(trx_state->_trx->operations.size() == 1);
				auto op = trx_state->_trx->operations[0];
				auto combine_op = op.get<coldhot_transfer_combine_sign_operation>();
				auto range = db().get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(o.coldhot_transfer_trx_id);
				set<transaction_id_type> sign_ids;
				for (auto sign_trx : boost::make_iterator_range(range.first, range.second)) {
					sign_ids.insert(sign_trx.current_id);
				}
				for (const auto & combine_trx_ids : o.signed_trx_ids) {
					auto sign_iter = sign_ids.find(combine_trx_ids);
					FC_ASSERT(sign_iter != sign_ids.end());
				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto crosschain_plugin = manager.get_crosschain_handle(o.asset_symbol);
				if (crosschain_plugin->valid_config()) {
					return void_result();
				}
				auto coldhot_trx = crosschain_plugin->turn_trxs(o.coldhot_trx_original_chain);
				FC_ASSERT(coldhot_trx.trxs.begin()->second.trx_id == o.original_trx_id);
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_transfer_combine_sign_evaluate::do_apply(const coldhot_transfer_combine_sign_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_coldhot_transaction(o.coldhot_transfer_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_combine_sign_operation>::value));
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto crosschain_plugin = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!crosschain_plugin->valid_config())
					return void_result();
				if (o.asset_symbol == "ETH" || o.asset_symbol.find("ERC") != o.asset_symbol.npos)
				{

				}
				else {
					crosschain_plugin->broadcast_transaction(o.coldhot_trx_original_chain);
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}

		void coldhot_transfer_combine_sign_evaluate::pay_fee() {

		}

		void_result coldhot_transfer_result_evaluate::do_evaluate(const coldhot_transfer_result_operation& o) {
			try {
				database & d = db();
				auto& originaldb = d.get_index_type<coldhot_transfer_index>().indices().get<by_original_trxid_optype>();
				auto combine_op_number = uint64_t(operation::tag<coldhot_transfer_combine_sign_operation>::value);
				string crosschain_trx_id = o.coldhot_trx_original_chain.trx_id;
				if (o.coldhot_trx_original_chain.asset_symbol == "ETH" || o.coldhot_trx_original_chain.asset_symbol.find("ERC") != o.coldhot_trx_original_chain.asset_symbol.npos)
				{
					if (o.coldhot_trx_original_chain.trx_id.find('|') != o.coldhot_trx_original_chain.trx_id.npos)
					{
						auto pos = o.coldhot_trx_original_chain.trx_id.find('|');
						crosschain_trx_id = o.coldhot_trx_original_chain.trx_id.substr(pos + 1);
					}
				}
				auto combine_trx_iter = originaldb.find(boost::make_tuple(crosschain_trx_id, combine_op_number));
				FC_ASSERT(combine_trx_iter != originaldb.end(), "combine sign trx doesn`t exist");
				auto& coldhot_db_objs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(combine_trx_iter->relate_trx_id);
				FC_ASSERT(tx_coldhot_without_sign_iter != coldhot_db_objs.end(), "without sign trx doesn`t exist");
				auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
				FC_ASSERT(tx_coldhot_original_iter != coldhot_db_objs.end(), "coldhot transfer trx doesn`t exist");
				auto & deposit_db = d.get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(crosschain_trx_id);
				if (deposit_to_link_trx != deposit_db.end()) {
					if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
						FC_ASSERT("deposit to link transaction doesn`t exist");
					}
				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.coldhot_trx_original_chain.asset_symbol))
					return void_result();
				auto crosschain_plugin = manager.get_crosschain_handle(o.coldhot_trx_original_chain.asset_symbol);
				if (crosschain_plugin->valid_config()) {
					return void_result();
				}
				auto obj = db().get_asset(o.coldhot_trx_original_chain.asset_symbol);
				FC_ASSERT(obj.valid());
				auto descrip = obj->options.description;
				bool bCheckTransactionValid = false;
				if (o.coldhot_trx_original_chain.asset_symbol.find("ERC") != o.coldhot_trx_original_chain.asset_symbol.npos) {
					auto temp_hdtx = o.coldhot_trx_original_chain;
					temp_hdtx.asset_symbol = temp_hdtx.asset_symbol + '|' + descrip;
					bCheckTransactionValid = crosschain_plugin->validate_link_trx(temp_hdtx);
				}
				else {
					bCheckTransactionValid = crosschain_plugin->validate_link_trx(o.coldhot_trx_original_chain);
				}
				FC_ASSERT(bCheckTransactionValid, "This transaction doesnt valid");
				//crosschain_plugin->validate_link_trx(o.coldhot_trx_original_chain);
				return void_result();

			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result coldhot_transfer_result_evaluate::do_apply(const coldhot_transfer_result_operation& o) {
			try {
				auto& originaldb = db().get_index_type<coldhot_transfer_index>().indices().get<by_original_trxid_optype>();
				auto combine_op_number = uint64_t(operation::tag<coldhot_transfer_combine_sign_operation>::value);
				string crosschain_trx_id = o.coldhot_trx_original_chain.trx_id;
				if (o.coldhot_trx_original_chain.asset_symbol == "ETH" || o.coldhot_trx_original_chain.asset_symbol.find("ERC") != o.coldhot_trx_original_chain.asset_symbol.npos)
				{
					if (o.coldhot_trx_original_chain.trx_id.find('|') != o.coldhot_trx_original_chain.trx_id.npos)
					{
						auto pos = o.coldhot_trx_original_chain.trx_id.find('|');
						crosschain_trx_id = o.coldhot_trx_original_chain.trx_id.substr(pos + 1);
					}
				}
				auto combine_trx_iter = originaldb.find(boost::make_tuple(crosschain_trx_id, combine_op_number));
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_coldhot_transaction(combine_trx_iter->relate_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_result_operation>::value));
				db().adjust_crosschain_confirm_trx(o.coldhot_trx_original_chain);

				//after this ,the current circulaiton of this asset will decline , so we need to change 
				//that how to pay  fees is considerable,maybe just borrowing from fees pool is good idea....
				//even though there are no fees in pool, we can still borrow, set a minus
				//if there are enough withraw requests, fees in fees pool will become active.
				auto& dyn_asset = db().get_asset(o.coldhot_trx_original_chain.asset_symbol)->dynamic_asset_data_id(db());
				db().modify(dyn_asset, [&o](asset_dynamic_data_object& d) {
					d.current_supply -= d.fee_pool;
				});
				db().modify_current_collected_fee(-asset(dyn_asset.fee_pool, db().get_asset(o.coldhot_trx_original_chain.asset_symbol)->id));

				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}

		void coldhot_transfer_result_evaluate::pay_fee() {
			
		}
		void_result coldhot_cancel_transafer_transaction_evaluate::do_evaluate(const coldhot_cancel_transafer_transaction_operation& o) {
			try {
				database & d = db();
				auto & coldhot_db = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_iter = coldhot_db.find(o.trx_id);
				FC_ASSERT(coldhot_iter != coldhot_db.end());
				FC_ASSERT(coldhot_iter->curret_trx_state == coldhot_without_sign_trx_uncreate, "Coldhot transaction has been create");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_cancel_transafer_transaction_evaluate::do_apply(const coldhot_cancel_transafer_transaction_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_coldhot_transaction(o.trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_cancel_transafer_transaction_operation>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void coldhot_cancel_transafer_transaction_evaluate::pay_fee() {

		}
		void_result coldhot_cancel_uncombined_trx_evaluate::do_evaluate(const coldhot_cancel_uncombined_trx_operaion& o) {
			try {
				database & d = db();
				auto & coldhot_db = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_iter = coldhot_db.find(o.trx_id);
				FC_ASSERT(coldhot_iter != coldhot_db.end());
				FC_ASSERT(coldhot_iter->curret_trx_state == coldhot_without_sign_trx_create, "Coldhot transaction state error");
				auto source_trx = coldhot_db.find(coldhot_iter->relate_trx_id);
				FC_ASSERT(source_trx != coldhot_db.end(),"source trx exist error");
				if(trx_state->_trx==nullptr)
					return  void_result();
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))

		}
		void_result coldhot_cancel_uncombined_trx_evaluate::do_apply(const coldhot_cancel_uncombined_trx_operaion& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr,"trx_state->_trx should not be nullptr");
				db().adjust_coldhot_transaction(o.trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_cancel_uncombined_trx_operaion>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void coldhot_cancel_uncombined_trx_evaluate::pay_fee() {

		}
		void_result coldhot_cancel_combined_trx_evaluate::do_evaluate(const coldhot_cancel_combined_trx_operaion& o) {
			try {
				database & d = db();
				auto & coldhot_db = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_iter = coldhot_db.find(o.fail_trx_id);
				FC_ASSERT(coldhot_iter != coldhot_db.end());
				FC_ASSERT(coldhot_iter->curret_trx_state == coldhot_combine_trx_create, "Coldhot transaction state error");
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.fail_trx_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 720 < current_blockNum);
				auto source_trx = coldhot_db.find(coldhot_iter->relate_trx_id);
				FC_ASSERT(source_trx != coldhot_db.end(), "source trx exist error");
				if (trx_state->_trx == nullptr)
					return  void_result();
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))

		}
		void_result coldhot_cancel_combined_trx_evaluate::do_apply(const coldhot_cancel_combined_trx_operaion& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr, "trx_state->_trx should not be nullptr");
				db().adjust_coldhot_transaction(o.fail_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_cancel_combined_trx_operaion>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void coldhot_cancel_combined_trx_evaluate::pay_fee() {

		}
		void_result eth_cancel_coldhot_fail_trx_evaluate::do_evaluate(const eth_cancel_coldhot_fail_trx_operaion& o) {
			try {
				database & d = db();
				const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_id);
				FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				auto & coldhot_db = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_iter = coldhot_db.find(o.fail_trx_id);
				FC_ASSERT(coldhot_iter != coldhot_db.end());
				FC_ASSERT(coldhot_iter->curret_trx_state == coldhot_eth_guard_sign, "Coldhot transaction state error");
				auto coldhot_without_sign_iter = coldhot_db.find(coldhot_iter->relate_trx_id);
				FC_ASSERT(coldhot_without_sign_iter != coldhot_db.end());
				auto source_trx = coldhot_db.find(coldhot_iter->relate_trx_id);
				FC_ASSERT(source_trx != coldhot_db.end(), "source trx exist error");
				if (trx_state->_trx == nullptr)
					return  void_result();
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				FC_ASSERT(coldhot_iter->current_trx.operations.size() == 1, "operation size error");
				const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
				const auto trx_history_iter = trx_history_db.find(o.fail_trx_id);
				FC_ASSERT(trx_history_iter != trx_history_db.end());
				auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
				FC_ASSERT(trx_history_iter->block_num + 720 < current_blockNum);
				auto op = coldhot_iter->current_trx.operations[0];
				FC_ASSERT(op.which() == operation::tag<eths_coldhot_guard_sign_final_operation>::value, "operation type error");
				auto eths_guard_sign_final_op = op.get<eths_coldhot_guard_sign_final_operation>();
				FC_ASSERT(eths_guard_sign_final_op.chain_type == "ETH" || eths_guard_sign_final_op.chain_type.find("ERC") != eths_guard_sign_final_op.chain_type.npos);
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(eths_guard_sign_final_op.chain_type))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(eths_guard_sign_final_op.chain_type));
				if (!hdl->valid_config())
					return void_result();
				auto eth_fail_transaction_id = eths_guard_sign_final_op.signed_crosschain_trx_id;
				auto eth_transaction = hdl->transaction_query(eth_fail_transaction_id);
				FC_ASSERT(eth_transaction.contains("respit_trx"));
				FC_ASSERT(eth_transaction.contains("source_trx"));
				auto respit_trx = eth_transaction["respit_trx"].get_object();
				auto eth_source_trx = eth_transaction["source_trx"].get_object();
				FC_ASSERT(respit_trx.contains("logs"));
				FC_ASSERT(respit_trx.contains("gasUsed"));
				FC_ASSERT(eth_source_trx.contains("gas"));
				auto receipt_logs = respit_trx["logs"].get_array();
				FC_ASSERT(receipt_logs.size() == 0, "this trasnaction not fail");
				FC_ASSERT(eth_source_trx["gas"].as_string() == respit_trx["gasUsed"].as_string());
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))

		}
		void_result eth_cancel_coldhot_fail_trx_evaluate::do_apply(const eth_cancel_coldhot_fail_trx_operaion& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr, "trx_state->_trx should not be nullptr");
				db().adjust_coldhot_transaction(o.fail_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eth_cancel_coldhot_fail_trx_operaion>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void eth_cancel_coldhot_fail_trx_evaluate::pay_fee() {

		}
	}
}
