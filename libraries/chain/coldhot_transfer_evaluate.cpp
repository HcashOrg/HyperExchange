#include <graphene/chain/coldhot_transfer_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>

namespace graphene {
	namespace chain {
		void_result coldhot_transfer_evaluate::do_evaluate(const coldhot_transfer_operation& o) {
			try {
				database& d = db();
				auto & asset_idx = d.get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				FC_ASSERT(asset_itr != asset_idx.end());
				FC_ASSERT(asset_itr->symbol == o.asset_symbol);
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
				FC_ASSERT((o.multi_account_deposit != o.multi_account_withdraw), "Cant Transfer account which is accually same account");
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto current_trx_id = trx_state->_trx->id();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(current_trx_id);
				FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "This coldhot Transaction exist");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_transfer_evaluate::do_apply(const coldhot_transfer_operation& o) {
			try {
				database& d = db();
				d.adjust_coldhot_transaction(transaction_id_type(), trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_operation>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}

		void coldhot_transfer_evaluate::pay_fee(){

		}

		void_result coldhot_transfer_without_sign_evaluate::do_evaluate(const coldhot_transfer_without_sign_operation& o) {
			try {
				database& d = db();
				//check coldhot transfer trx whether exist
				auto& coldhot_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto coldhot_tx_iter = coldhot_tx_dbs.find(o.coldhot_trx_id);
				FC_ASSERT(coldhot_tx_iter != coldhot_tx_dbs.end(), "coldhot original trx doesn`t exist");
				//check without sign trx whether exist
				auto& coldhot_without_sign_tx_dbs = d.get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>();
				auto coldhot_without_sign_tx_iter = coldhot_without_sign_tx_dbs.find(trx_state->_trx->id());
				FC_ASSERT(coldhot_without_sign_tx_iter == coldhot_without_sign_tx_dbs.end(), "coldhot trx without sign has been created");
				
				auto& instance = graphene::crosschain::crosschain_manager::get_instance();
				if (!instance.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto crosschain_handle = instance.get_crosschain_handle(o.asset_symbol);
				if (!crosschain_handle->valid_config())
					return void_result();
				auto created_trx = crosschain_handle->turn_trxs(o.coldhot_trx_original_chain);
				
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
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		void_result coldhot_transfer_with_sign_evaluate::do_apply(const coldhot_transfer_with_sign_operation& o) {
			try {
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
				db().adjust_coldhot_transaction(o.coldhot_transfer_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_transfer_combine_sign_operation>::value));
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto crosschain_plugin = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!crosschain_plugin->valid_config())
					return void_result();
				crosschain_plugin->broadcast_transaction(o.coldhot_trx_original_chain);
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
				auto combine_trx_iter = originaldb.find(boost::make_tuple(o.coldhot_trx_original_chain.trx_id, combine_op_number));
				FC_ASSERT(combine_trx_iter != originaldb.end(), "combine sign trx doesn`t exist");
				auto& coldhot_db_objs = d.get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(combine_trx_iter->relate_trx_id);
				FC_ASSERT(tx_coldhot_without_sign_iter != coldhot_db_objs.end(), "without sign trx doesn`t exist");
				auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
				FC_ASSERT(tx_coldhot_original_iter != coldhot_db_objs.end(), "coldhot transfer trx doesn`t exist");
				auto & deposit_db = d.get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(o.coldhot_trx_original_chain.trx_id);
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
				crosschain_plugin->validate_link_trx(o.coldhot_trx_original_chain);
				return void_result();

			}FC_CAPTURE_AND_RETHROW((o))
		}
		void_result coldhot_transfer_result_evaluate::do_apply(const coldhot_transfer_result_operation& o) {
			try {
				auto& originaldb = db().get_index_type<coldhot_transfer_index>().indices().get<by_original_trxid_optype>();
				auto combine_op_number = uint64_t(operation::tag<coldhot_transfer_combine_sign_operation>::value);
				auto combine_trx_iter = originaldb.find(boost::make_tuple(o.coldhot_trx_original_chain.trx_id, combine_op_number));
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
				db().modify_current_collected_fee(-dyn_asset.fee_pool);

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
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))

		}
		void_result coldhot_cancel_uncombined_trx_evaluate::do_apply(const coldhot_cancel_uncombined_trx_operaion& o) {
			try {
				db().adjust_coldhot_transaction(o.trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<coldhot_cancel_uncombined_trx_operaion>::value));
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o))
		}
		void coldhot_cancel_uncombined_trx_evaluate::pay_fee() {

		}
	}
}
