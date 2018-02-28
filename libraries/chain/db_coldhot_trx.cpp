#include <graphene/chain/database.hpp>
#include <graphene/chain/coldhot_transfer_object.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_coldhot_transaction(transaction_id_type relate_trx_id, transaction_id_type current_trx_id, signed_transaction current_trx, uint64_t op_type) {
			try {
				if (op_type == operation::tag<coldhot_transfer_operation>::value) {
// 					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 					auto coldhot_tx_iter = coldhot_tx_dbs.find(current_trx_id);
// 					FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "This coldhot Transaction exist");
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = transaction_id_type();
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_without_sign_trx_uncreate;
					});
				}
				else if (op_type == operation::tag<coldhot_transfer_without_sign_operation>::value){
					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto coldhot_tx_iter = coldhot_tx_dbs.find(relate_trx_id);
					//FC_ASSERT(coldhot_tx_iter != coldhot_tx_dbs.end(), "coldhot original trx doesn`t exist");
					//check without sign trx exist
// 					auto& coldhot_relate_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>();
// 					auto coldhot_relate_tx_iter = coldhot_relate_tx_dbs.find(current_trx_id);
// 					FC_ASSERT(coldhot_relate_tx_iter == coldhot_relate_tx_dbs.end(), "coldhot trx without sign has been created");
					modify(*coldhot_tx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_without_sign_trx_create;
					});
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_without_sign_trx_create;
					});
				}
				else if (op_type == operation::tag<coldhot_transfer_with_sign_operation>::value) {
// 					auto& coldhot_original_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 					auto coldhot_original_tx_iter = coldhot_original_tx_dbs.find(relate_trx_id);
// 					FC_ASSERT(coldhot_original_tx_iter != coldhot_original_tx_dbs.end(), "coldhot original trx doesn`t exist");
// 					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 					auto coldhot_tx_iter = coldhot_tx_dbs.find(current_trx_id);
// 					FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "coldhot trx with this sign has been created");
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_sign_trx;
					});
				}
				else if (op_type == operation::tag<coldhot_transfer_combine_sign_operation>::value){
// 					auto& coldhot_relate_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 					auto coldhot_relate_tx_iter = coldhot_relate_tx_dbs.find(relate_trx_id);
// 					FC_ASSERT(coldhot_relate_tx_iter != coldhot_relate_tx_dbs.end(), "coldhot original trx doesn`t exist");
// 					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 					auto coldhot_tx_iter = coldhot_tx_dbs.find(current_trx_id);
// 					FC_ASSERT(coldhot_tx_iter == coldhot_tx_dbs.end(), "coldhot trx combine has been created");
// 					auto coldhot_relate_original_iter = coldhot_tx_dbs.find(coldhot_relate_tx_iter->relate_trx_id);
// 					FC_ASSERT(coldhot_relate_original_iter != coldhot_tx_dbs.end(), "coldhot original relate trx doesn`t exist");
					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto coldhot_without_sign_trx_iter = coldhot_tx_dbs.find(relate_trx_id);
					auto coldhot_transfer_trx_iter = coldhot_tx_dbs.find(coldhot_without_sign_trx_iter->relate_trx_id);
					modify(*coldhot_transfer_trx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_combine_trx_create;
					});
					modify(*coldhot_without_sign_trx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_combine_trx_create;
					});
					//auto sign_range = get_index_type< coldhot_transfer_index > ().indices().get<by_relate_trxidstate>().equal_range(boost::make_tuple(relate_trx_id, coldhot_sign_trx));
					auto sign_range = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(relate_trx_id);
					for (auto sign_trx_obj : boost::make_iterator_range(sign_range.first,sign_range.second)){
						auto& sign_tx_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
						auto sign_tx_iter = sign_tx_db.find(sign_trx_obj.current_id);
						if (sign_tx_iter->curret_trx_state != coldhot_sign_trx){
							continue;
						}
						modify(*sign_tx_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_combine_trx_create;
						});
					}
// 					std::vector<coldhot_transfer_object> sign_trxs;
// 					get_index_type<coldhot_transfer_index >().inspect_all_objects([&](const object& o)
// 					{
// 						const coldhot_transfer_object& p = static_cast<const coldhot_transfer_object&>(o);
// 						if (p.curret_trx_state == coldhot_sign_trx && p.relate_trx_id == relate_trx_id) {
// 							sign_trxs.push_back(p);
// 						}
// 					});
// 					for (auto sign_trx_obj : sign_trxs) {
// 						auto& sign_tx_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 						auto sign_tx_iter = sign_tx_db.find(sign_trx_obj.current_id);
// 						modify(*sign_tx_iter, [&](coldhot_transfer_object& obj) {
// 							obj.curret_trx_state = coldhot_combine_trx_create;
// 						});
// 					}
					auto op = current_trx.operations[0];
					auto combine_op = op.get<coldhot_transfer_combine_sign_operation>();
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.original_trx_id = combine_op.original_trx_id;
						obj.curret_trx_state = coldhot_combine_trx_create;
					});
				}
				else if (op_type == operation::tag<coldhot_transfer_result_operation>::value){
					auto& coldhot_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(relate_trx_id);
					//FC_ASSERT(tx_coldhot_without_sign_iter != coldhot_db_objs.end(), "without sign trx doesn`t exist");
					auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
					//FC_ASSERT(tx_coldhot_original_iter != coldhot_db_objs.end(), "original trx doesn`t exist");

					modify(*tx_coldhot_without_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_confirm;
					});
					modify(*tx_coldhot_original_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_confirm;
					});
// 					std::vector<coldhot_transfer_object> sign_trxs;
// 					get_index_type<coldhot_transfer_index >().inspect_all_objects([&](const object& o)
// 					{
// 						const coldhot_transfer_object& p = static_cast<const coldhot_transfer_object&>(o);
// 						if (p.curret_trx_state == coldhot_combine_trx_create && p.relate_trx_id == relate_trx_id) {
// 							sign_trxs.push_back(p);
// 						}
// 					});
// 					for (auto sign_trx_obj : sign_trxs) {
// 						auto& sign_tx_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
// 						auto sign_tx_iter = sign_tx_db.find(sign_trx_obj.current_id);
// 						modify(*sign_tx_iter, [&](coldhot_transfer_object& obj) {
// 							obj.curret_trx_state = coldhot_transaction_confirm;
// 						});
// 					}
					auto combine_state_tx_range = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(relate_trx_id);
					for (auto combine_state_tx : boost::make_iterator_range(combine_state_tx_range.first, combine_state_tx_range.second)) {
						if (combine_state_tx.curret_trx_state != coldhot_combine_trx_create) {
							continue;
						}
						auto combine_state_tx_iter = coldhot_db_objs.find(combine_state_tx.current_id);
						modify(*combine_state_tx_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_transaction_confirm;
						});
					}
				}
			}FC_CAPTURE_AND_RETHROW((relate_trx_id)(current_trx_id))
		}
		void database::create_coldhot_transfer_trx(miner_id_type miner, fc::ecc::private_key pk) {
			auto coldhot_uncreate_range = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_without_sign_trx_uncreate);
			for (auto coldhot_transfer_trx : boost::make_iterator_range(coldhot_uncreate_range.first, coldhot_uncreate_range.second)) {
				try {
					auto coldhot_transfer_op = coldhot_transfer_trx.current_trx.operations[0];
					/*auto& propsal_op = coldhot_transfer_op.get<proposal_create_operation>();
					if (propsal_op.proposed_ops.size() != 1){
						continue;
					}*/
					
					//for (auto coldhot_transfer : propsal_op.proposed_ops){
						auto coldhot_op = coldhot_transfer_op.get<coldhot_transfer_operation>();
						auto & manager = graphene::crosschain::crosschain_manager::get_instance();
						auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
						coldhot_transfer_without_sign_operation trx_op;
						std::map<string, string> dest_info;
						dest_info[std::string(coldhot_op.multi_account_deposit)] = coldhot_op.amount;
						trx_op.coldhot_trx_original_chain = crosschain_plugin->create_multisig_transaction(std::string(coldhot_op.multi_account_withdraw),
								dest_info,
								coldhot_op.asset_symbol,
								coldhot_op.memo,"");
						trx_op.coldhot_trx_id = coldhot_transfer_trx.current_id;
						trx_op.miner_broadcast = miner;
						trx_op.asset_symbol = coldhot_op.asset_symbol;
						trx_op.asset_id = coldhot_op.asset_id;
						optional<miner_object> miner_iter = get(miner);
						optional<account_object> account_iter = get(miner_iter->miner_account);
						trx_op.miner_address = account_iter->addr;

						signed_transaction tx;
						uint32_t expiration_time_offset = 0;
						auto dyn_props = get_dynamic_global_properties();
						operation temp = operation(trx_op);
						get_global_properties().parameters.current_fees->set_fee(temp);
						tx.set_reference_block(dyn_props.head_block_id);
						tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
						tx.operations.push_back(trx_op);
						tx.validate();
						tx.sign(pk, get_chain_id());
						push_transaction(tx);
					//}
				}
				catch (...) {
					continue;
				}
			}
		}
		void database::combine_coldhot_sign_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			auto coldhot_sign_range = get_index_type<coldhot_transfer_index>().indices().get<by_optype>().equal_range(uint64_t(operation::tag<coldhot_transfer_with_sign_operation>::value));
			map<transaction_id_type, set<transaction_id_type>> uncombine_trxs;
			for (auto coldhot_sign_tx : boost::make_iterator_range(coldhot_sign_range.first, coldhot_sign_range.second)){
				uncombine_trxs[coldhot_sign_tx.relate_trx_id].insert(coldhot_sign_tx.current_id);
			}
			fc::variant_object ad;
			for (auto & trxs : uncombine_trxs) {
				try {
					if (trxs.second.size() < ceil(float(get_global_properties().active_committee_members.size())*2.0 / 3.0)) {
						continue;
					}
					set<string> combine_signature;
					auto & tx_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					bool transaction_err = false;
					for (auto & sign_trx_id : trxs.second) {
						auto trx_itr = tx_db.find(sign_trx_id);
						if (trx_itr == tx_db.end() || trx_itr->current_trx.operations.size() < 1) {
							transaction_err = true;
							break;
						}
						auto op = trx_itr->current_trx.operations[0];
						if (op.which() != operation::tag<coldhot_transfer_with_sign_operation>::value)
						{
							continue;
						}
						auto coldhot_sign_op = op.get<coldhot_transfer_with_sign_operation>();
						combine_signature.insert(coldhot_sign_op.coldhot_transfer_sign);
					}
					if (transaction_err) {
						continue;
					}
					auto & tx_relate_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trxidstate>();
					auto relate_tx_iter = tx_relate_db.find(boost::make_tuple(trxs.first, coldhot_without_sign_trx_create));
					if (relate_tx_iter == tx_relate_db.end() || relate_tx_iter->current_trx.operations.size() < 1) {
						continue;
					}
					auto op = relate_tx_iter->current_trx.operations[0];
					auto coldhot_op = op.get<coldhot_transfer_without_sign_operation>();
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
					coldhot_transfer_combine_sign_operation trx_op;

					vector<string> guard_signs(combine_signature.begin(), combine_signature.end());
					trx_op.coldhot_trx_original_chain = crosschain_plugin->merge_multisig_transaction(coldhot_op.coldhot_trx_original_chain, guard_signs);
					ad = trx_op.coldhot_trx_original_chain;
					trx_op.asset_symbol = coldhot_op.asset_symbol;
					vector<transaction_id_type> vector_ids(trxs.second.begin(), trxs.second.end());
					trx_op.signed_trx_ids.swap(vector_ids);
					trx_op.miner_broadcast = miner;
					optional<miner_object> miner_iter = get(miner);
					optional<account_object> account_iter = get(miner_iter->miner_account);
					trx_op.miner_address = account_iter->addr;
					trx_op.coldhot_transfer_trx_id = relate_tx_iter->current_id;
					
					auto hdl_trx = crosschain_plugin->turn_trxs(trx_op.coldhot_trx_original_chain);
					trx_op.original_trx_id = hdl_trx.begin()->second.trx_id;
					signed_transaction tx;
					uint32_t expiration_time_offset = 0;
					auto dyn_props = get_dynamic_global_properties();
					operation temp = operation(trx_op);
					get_global_properties().parameters.current_fees->set_fee(temp);
					tx.set_reference_block(dyn_props.head_block_id);
					tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
					tx.operations.push_back(trx_op);
					tx.validate();
					tx.sign(pk, get_chain_id());
					push_transaction(tx);
				}FC_CAPTURE_AND_LOG((ad))
			}
		}
	}
}