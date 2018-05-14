#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_deposit_to_link_trx(const hd_trx& handled_trx) {
			try {
				auto & deposit_db = get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(handled_trx.trx_id);
				if (deposit_to_link_trx == deposit_db.end()) {
					create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
						obj.handle_trx = handled_trx;
						obj.handle_trx_id = handled_trx.trx_id;
						obj.acquired_transaction_state = acquired_trx_create;
					});
				}
				else {
// 					if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
// 						FC_ASSERT("deposit transaction exist");
// 					}
					modify(*deposit_to_link_trx, [&](acquired_crosschain_trx_object& obj) {
						obj.acquired_transaction_state = acquired_trx_create;
					});
				}

			}FC_CAPTURE_AND_RETHROW((handled_trx))
		}
		void database::adjust_crosschain_confirm_trx(const hd_trx& handled_trx) {
			try {
				auto & deposit_db = get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(handled_trx.trx_id);
				if (deposit_to_link_trx == deposit_db.end()) {
					create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
						obj.handle_trx = handled_trx;
						obj.handle_trx_id = handled_trx.trx_id;
						obj.acquired_transaction_state = acquired_trx_comfirmed;
					});
				}
				else {
// 					if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
// 						FC_ASSERT("deposit transaction exist");
// 					}
					modify(*deposit_to_link_trx, [&](acquired_crosschain_trx_object& obj) {
						obj.acquired_transaction_state = acquired_trx_comfirmed;
					});
				}

			}FC_CAPTURE_AND_RETHROW((handled_trx))
		}

		
		void database::adjust_crosschain_transaction(transaction_id_type relate_transaction_id,
			transaction_id_type transaction_id,
			signed_transaction real_transaction,
			uint64_t op_type,
			transaction_stata trx_state,
			vector<transaction_id_type> relate_transaction_ids
		) {

			try {
				if (op_type == operation::tag<crosschain_withdraw_evaluate::operation_type>::value) {
					auto op = real_transaction.operations[0];
					auto withdraw_op = op.get<crosschain_withdraw_operation>();
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = transaction_id_type();
						obj.transaction_id = transaction_id;
						obj.real_transaction = real_transaction;
						obj.without_link_account = string(withdraw_op.withdraw_account);
						obj.trx_state = withdraw_without_sign_trx_uncreate;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_without_sign_evaluate::operation_type>::value) {
					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					//auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_without_sign_trx_iter = trx_db.find(transaction_id);
					auto trx_crosschain_withdraw_iter = trx_db.find(relate_transaction_id);
					//std::cout << relate_transaction_id.str() << std::endl;
					//std::cout << (trx_iter_new == trx_db_new.end()) << std::endl;
					modify(*trx_crosschain_withdraw_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_without_sign_trx_create;
					});
					if (trx_without_sign_trx_iter == trx_db.end())
					{
						create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
							obj.op_type = op_type;
							obj.relate_transaction_id = relate_transaction_id;
							obj.real_transaction = real_transaction;
							obj.transaction_id = transaction_id;
							obj.all_related_origin_transaction_ids = relate_transaction_ids;
							obj.trx_state = withdraw_without_sign_trx_create;
						});
					}
				}
				else if (op_type == operation::tag<crosschain_withdraw_combine_sign_evaluate::operation_type>::value) {
					auto & tx_db_objs = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto tx_without_sign_iter = tx_db_objs.find(relate_transaction_id);
					auto tx_combine_sign_iter = tx_db_objs.find(transaction_id);
					//FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "without sign tx exist error");
					//FC_ASSERT(tx_combine_sign_iter == tx_db_objs.end(), "combine sign tx has create");
					//for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
					//	auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
					//	FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
					//}
					auto op = real_transaction.operations[0];
					auto combine_op = op.get<crosschain_withdraw_combine_sign_operation>();
					modify(*tx_without_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_combine_trx_create;
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = withdraw_combine_trx_create;
							obj.crosschain_trx_id = combine_op.crosschain_trx_id;
						});
					}
// 					auto& sign_trx_db = get_index_type< crosschain_trx_index >().indices().get<by_trx_relate_type_stata>();
// 					for (auto one_sign_trx : sign_trx_db)
// 					{
// 						if ( one_sign_trx.trx_state == withdraw_sign_trx)
// 						{
// 							std::cout << one_sign_trx.relate_transaction_id.str() << std::endl;
// 						}
// 					}

					const auto& sign_range = get_index_type< crosschain_trx_index >().indices().get<by_relate_trx_id>().equal_range(relate_transaction_id);
					int count = 0;
					std::for_each(sign_range.first, sign_range.second, [&](const crosschain_trx_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						count++;
						if (sign_iter->trx_state == withdraw_sign_trx) {
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_combine_trx_create;
							});
						}
					});
					//std::cout << "count: " << count << std::endl;
					/*for (auto sign_tx : boost::make_iterator_range(sign_range.first, sign_range.second)) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						if(sign_iter->trx_state == withdraw_sign_trx){
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_combine_trx_create;
							});
						}
					}*/
					
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.crosschain_trx_id = combine_op.crosschain_trx_id;
						obj.trx_state = withdraw_combine_trx_create;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_with_sign_evaluate::operation_type>::value) {
// 					auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
// 					auto trx_itr_relate = trx_db_relate.find(relate_transaction_id);
// 					FC_ASSERT(trx_itr_relate != trx_db_relate.end(), "Source trx doesnt exist");
// 
// 					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
// 					auto trx_itr = trx_db.find(transaction_id);
// 					FC_ASSERT(trx_itr == trx_db.end(), "This sign tex is exist");
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.trx_state = withdraw_sign_trx;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_result_evaluate::operation_type>::value) {
					auto & tx_db_objs = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto tx_without_sign_iter = tx_db_objs.find(relate_transaction_id);
					FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "user cross chain tx exist error");
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
					}
					modify(*tx_without_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_transaction_confirm;
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = withdraw_transaction_confirm;
						});
					}

					const auto & combine_state_range = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(relate_transaction_id );
					int count = 0;
					std::for_each(combine_state_range.first, combine_state_range.second, [&](const crosschain_trx_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						count++;
						if (sign_iter->trx_state == withdraw_combine_trx_create) {
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_transaction_confirm;
							});
						}
					});
				}
			}FC_CAPTURE_AND_RETHROW((relate_transaction_id)(transaction_id))
		}
		void database::create_result_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			try {

				//need to check if can be created
				auto check_point_1 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_without_sign_trx_create);
				auto check_point_2 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_sign_trx);
				flat_set<string> asset_set;
				for (auto check_point : boost::make_iterator_range(check_point_1.first, check_point_1.second))
				{
					auto op = check_point.real_transaction.operations[0];
					if (op.which() == operation::tag<crosschain_withdraw_without_sign_operation>().value)
					{
						auto withop = op.get<crosschain_withdraw_without_sign_operation>();
						asset_set.insert(withop.asset_symbol);
					}
				}

				for (auto check_point : boost::make_iterator_range(check_point_2.first, check_point_2.second))
				{
					auto op = check_point.real_transaction.operations[0];
					if (op.which() == operation::tag<crosschain_withdraw_with_sign_operation>().value)
					{
						auto withop = op.get<crosschain_withdraw_with_sign_operation>();
						asset_set.insert(withop.asset_symbol);
					}

				}

				auto withdraw_range = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_without_sign_trx_uncreate);
				std::map<asset_id_type, std::map<std::string, std::string>> dest_info;
				std::map<std::string, std::string> memo_info;
				std::map<std::string, vector<transaction_id_type>> ccw_trx_ids;
				map<asset_id_type, double> fee;
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				for (auto & cross_chain_trx : boost::make_iterator_range(withdraw_range.first,withdraw_range.second)) {
					if (cross_chain_trx.real_transaction.id() != cross_chain_trx.transaction_id || cross_chain_trx.real_transaction.operations.size() < 1) {
						continue;
					}
					auto op = cross_chain_trx.real_transaction.operations[0];
					auto withop = op.get<crosschain_withdraw_evaluate::operation_type>();
					std::map<std::string, std::string> temp_map;
					std::vector<transaction_id_type> temp_vector;
					if (dest_info.count(withop.asset_id)>0)
					{
						temp_map = dest_info[withop.asset_id];
						temp_vector = ccw_trx_ids[withop.asset_symbol];
					}
					optional<asset_object> opt_asset = get(withop.asset_id);
					if (!opt_asset.valid()) {
						continue;
					}
					auto hdl = manager.get_crosschain_handle(std::string(withop.asset_symbol));
					if (!hdl->valid_config()) {
						continue;
					}
					bool valid_address = hdl->validate_address(withop.crosschain_account);
					if (!valid_address) {
						continue;
					}
					auto crosschain_fee = opt_asset->amount_to_string(opt_asset->dynamic_data(*this).fee_pool);

					auto temp_amount = fc::variant(withop.amount).as_double()- fc::variant(crosschain_fee).as_double();
					fee[withop.asset_id] += fc::variant(crosschain_fee).as_double();
						
					if (temp_map.count(withop.crosschain_account)>0){
						temp_map[withop.crosschain_account] = fc::to_string(to_double(temp_map[withop.crosschain_account]) + temp_amount).c_str();
					}
					else{
						temp_map[withop.crosschain_account] = fc::variant(temp_amount).as_string();
					}
					temp_vector.push_back(cross_chain_trx.real_transaction.id());
					dest_info[withop.asset_id] = temp_map;
					ccw_trx_ids[withop.asset_symbol] = temp_vector;
					memo_info[withop.asset_symbol] = withop.memo;
				}

				for (auto& one_asset : dest_info)
				{
					optional<asset_object> opt_asset = get(one_asset.first);
					if (!opt_asset.valid())
					{
						continue;
					}
					auto asset_symbol = opt_asset->symbol;
					if (asset_set.find(asset_symbol) != asset_set.end())
						continue;

					auto hdl = manager.get_crosschain_handle(std::string(asset_symbol));
					crosschain_withdraw_without_sign_operation trx_op;
					multisig_account_pair_object multi_account_obj;

					get_index_type<multisig_account_pair_index >().inspect_all_objects([&](const object& o)
					{
						const multisig_account_pair_object& p = static_cast<const multisig_account_pair_object&>(o);
						if (p.chain_type == asset_symbol) {
							if (multi_account_obj.effective_block_num < p.effective_block_num && p.effective_block_num <= head_block_num()) {
								multi_account_obj = p;
							}
						}
					});
					//auto multisign_hot = multisign_db.find()
					FC_ASSERT(multi_account_obj.bind_account_cold != multi_account_obj.bind_account_hot);
					/*std::map<const std::string, const std::string> temp_map;
					for (auto & itr : one_asset.second)
					{
					const std::string key = itr.first;
					const std::string value = itr.second;
					temp_map.insert(std::make_pair(key,value));
					}*/
					//the average fee is 0.001
					FC_ASSERT(fee.count(one_asset.first)>0);
					char temp_fee[1024];
					string format = "%."+fc::variant(opt_asset->precision).as_string()+"f";
					std::sprintf(temp_fee, format.c_str(), fee[one_asset.first]);
					auto t_fee = opt_asset->amount_from_string(graphene::utilities::remove_zero_for_str_amount(temp_fee));
					trx_op.withdraw_source_trx = hdl->create_multisig_transaction(multi_account_obj.bind_account_hot, one_asset.second, asset_symbol, memo_info[asset_symbol]);
					auto hdtrxs = hdl->turn_trxs(trx_op.withdraw_source_trx);
					memset(temp_fee,0,1024);
					std::sprintf(temp_fee, format.c_str(), hdtrxs.fee);
					auto cross_fee = opt_asset->amount_from_string(graphene::utilities::remove_zero_for_str_amount(temp_fee));
					trx_op.ccw_trx_ids = ccw_trx_ids[asset_symbol];
					//std::cout << trx_op.ccw_trx_id.str() << std::endl;
					trx_op.miner_broadcast = miner;
					trx_op.asset_symbol = asset_symbol;
					trx_op.asset_id = opt_asset->id;
					optional<miner_object> miner_iter = get(miner);
					optional<account_object> account_iter = get(miner_iter->miner_account);
					trx_op.miner_address = account_iter->addr;
					trx_op.crosschain_fee = t_fee-cross_fee;
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
					try {
						push_transaction(tx);
					}FC_CAPTURE_AND_LOG((0));
				}
			}FC_CAPTURE_AND_LOG((0))
		}
		void database::create_acquire_crosschhain_transaction(miner_id_type miner, fc::ecc::private_key pk){
			try {
				map<string, vector<acquired_crosschain_trx_object>> acquired_crosschain_trx;
				get_index_type<acquired_crosschain_index>().inspect_all_objects([&](const object& o) {
					const acquired_crosschain_trx_object& p = static_cast<const acquired_crosschain_trx_object&>(o);
					if (p.acquired_transaction_state == acquired_trx_uncreate) {
						acquired_crosschain_trx[p.handle_trx.asset_symbol].push_back(p);
					}
				});

				for (auto & acquired_trxs : acquired_crosschain_trx) {

					set<string> multi_account_obj_hot;
					set<string> multi_account_obj_cold;
					auto multi_account_range = get_index_type<multisig_account_pair_index>().indices().get<by_chain_type>().equal_range(acquired_trxs.first);
					for (auto multi_account_obj : boost::make_iterator_range(multi_account_range.first, multi_account_range.second)) {
						multi_account_obj_hot.insert(multi_account_obj.bind_account_hot);
						multi_account_obj_cold.insert(multi_account_obj.bind_account_cold);
					}
					/*get_index_type<multisig_account_pair_index >().inspect_all_objects([&](const object& o)
					{
					const multisig_account_pair_object& p = static_cast<const multisig_account_pair_object&>(o);
					if (p.chain_type == acquired_trxs.first) {
					multi_account_obj_hot.insert(p.bind_account_hot);
					}
					});*/
					for (auto acquired_trx : acquired_trxs.second) {
						bool multi_account_withdraw_cold = false;
						bool multi_account_deposit_cold = false;
						bool multi_account_withdraw_hot = false;
						bool multi_account_deposit_hot = false;
						auto to_iter_cold = multi_account_obj_cold.find(acquired_trx.handle_trx.to_account);
						auto from_iter_cold = multi_account_obj_cold.find(acquired_trx.handle_trx.from_account);
						int multi_account_count = 0;
						if (to_iter_cold != multi_account_obj_cold.end()) {
							multi_account_deposit_cold = true;
							++multi_account_count;
						}
						if (from_iter_cold != multi_account_obj_cold.end()) {
							multi_account_withdraw_cold = true;
							++multi_account_count;
						}
						auto to_iter_hot = multi_account_obj_hot.find(acquired_trx.handle_trx.to_account);
						auto from_iter_hot = multi_account_obj_hot.find(acquired_trx.handle_trx.from_account);
						if (to_iter_hot != multi_account_obj_hot.end()) {
							multi_account_deposit_hot = true;
							++multi_account_count;
						}
						if (from_iter_hot != multi_account_obj_hot.end()) {
							multi_account_withdraw_hot = true;
							++multi_account_count;
						}
						if (multi_account_count == 2) {
							coldhot_transfer_result_operation op;
							op.coldhot_trx_original_chain = acquired_trx.handle_trx;
							op.miner_broadcast = miner;
							optional<miner_object> miner_iter = get(miner);
							optional<account_object> account_iter = get(miner_iter->miner_account);
							op.miner_address = account_iter->addr;
							signed_transaction tx;
							uint32_t expiration_time_offset = 0;
							auto dyn_props = get_dynamic_global_properties();
							operation temp = operation(op);
							get_global_properties().parameters.current_fees->set_fee(temp);
							tx.set_reference_block(dyn_props.head_block_id);
							tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
							tx.operations.push_back(op);
							tx.validate();
							tx.sign(pk, get_chain_id());
							try {
								push_transaction(tx);
							}FC_CAPTURE_AND_LOG((0));
							
							continue;
						}
						if (multi_account_deposit_hot) {
							crosschain_record_operation op;
							auto & asset_iter = get_index_type<asset_index>().indices().get<by_symbol>();
							auto asset_itr = asset_iter.find(acquired_trx.handle_trx.asset_symbol);
							if (asset_itr == asset_iter.end()) {
								continue;
							}
							op.asset_id = asset_itr->id;
							op.asset_symbol = acquired_trx.handle_trx.asset_symbol;
							op.cross_chain_trx = acquired_trx.handle_trx;
							op.miner_broadcast = miner;
							optional<miner_object> miner_iter = get(miner);
							optional<account_object> account_iter = get(miner_iter->miner_account);
							op.miner_address = account_iter->addr;
							signed_transaction tx;
							uint32_t expiration_time_offset = 0;
							auto dyn_props = get_dynamic_global_properties();
							operation temp = operation(op);
							get_global_properties().parameters.current_fees->set_fee(temp);
							tx.set_reference_block(dyn_props.head_block_id);
							tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
							tx.operations.push_back(op);
							tx.validate();
							tx.sign(pk, get_chain_id());
							try {
								push_transaction(tx);
							}FC_CAPTURE_AND_LOG((0));
							continue;
						}
						else if (multi_account_withdraw_hot) {
							crosschain_withdraw_result_operation op;
							auto& originaldb = get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
							auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
							auto combine_trx_iter = originaldb.find(boost::make_tuple(acquired_trx.handle_trx.trx_id, combine_op_number));
							if (combine_trx_iter == originaldb.end())
							{
								continue;
							}
								
							op.cross_chain_trx = acquired_trx.handle_trx;
							op.miner_broadcast = miner;
							optional<miner_object> miner_iter = get(miner);
							optional<account_object> account_iter = get(miner_iter->miner_account);
							op.miner_address = account_iter->addr;
							signed_transaction tx;
							uint32_t expiration_time_offset = 0;
							auto dyn_props = get_dynamic_global_properties();
							operation temp = operation(op);
							get_global_properties().parameters.current_fees->set_fee(temp);
							tx.set_reference_block(dyn_props.head_block_id);
							tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
							tx.operations.push_back(op);
							tx.validate();
							tx.sign(pk, get_chain_id());
							try {
								push_transaction(tx);
							}FC_CAPTURE_AND_LOG((0));
							continue;
						}
					}
				}
			}FC_CAPTURE_AND_LOG((0))
			
		}
		void database::combine_sign_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			try {
				map<transaction_id_type, vector<transaction_id_type>> uncombine_trxs_counts;
				get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& o)
				{
					const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(o);
					if (p.trx_state == withdraw_sign_trx && p.relate_transaction_id != transaction_id_type()) {
						uncombine_trxs_counts[p.relate_transaction_id].push_back(p.transaction_id);
					}
				});
				for (auto & trxs : uncombine_trxs_counts) {
					//TODO : Use macro instead this magic number
					if (trxs.second.size() < ceil(float(get_global_properties().active_committee_members.size())*2.0/3.0)) {
						continue;
					}
					set<string> combine_signature;
					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					bool transaction_err = false;
					for (auto & sign_trx_id : trxs.second) {
						auto trx_itr = trx_db.find(sign_trx_id);

						if (trx_itr == trx_db.end() || trx_itr->real_transaction.operations.size() < 1) {
							transaction_err = true;
							break;
						}
						auto op = trx_itr->real_transaction.operations[0];
						if (op.which() != operation::tag<crosschain_withdraw_with_sign_evaluate::operation_type>::value)
						{
							continue;
						}
						auto with_sign_op = op.get<crosschain_withdraw_with_sign_evaluate::operation_type>();
						combine_signature.insert(with_sign_op.ccw_trx_signature);
					}
					if (transaction_err) {
						continue;
					}
					auto& trx_new_db = get_index_type<crosschain_trx_index>().indices().get<by_trx_type_state>();
					auto trx_itr = trx_new_db.find(boost::make_tuple(trxs.first, withdraw_without_sign_trx_create));
					if (trx_itr == trx_new_db.end() || trx_itr->real_transaction.operations.size() < 1) {
						continue;
					}
					auto op = trx_itr->real_transaction.operations[0];
					auto with_sign_op = op.get<crosschain_withdraw_without_sign_evaluate::operation_type>();
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					auto hdl = manager.get_crosschain_handle(std::string(with_sign_op.asset_symbol));
					crosschain_withdraw_combine_sign_operation trx_op;
					vector<string> guard_signed(combine_signature.begin(), combine_signature.end());
					trx_op.cross_chain_trx = hdl->merge_multisig_transaction(with_sign_op.withdraw_source_trx, guard_signed);
					trx_op.asset_symbol = with_sign_op.asset_symbol;
					trx_op.signed_trx_ids.swap(trxs.second);
					trx_op.crosschain_fee = with_sign_op.crosschain_fee;
					trx_op.miner_broadcast = miner;
					trx_op.crosschain_trx_id = hdl->turn_trxs(trx_op.cross_chain_trx).trxs.begin()->second.trx_id;
					optional<miner_object> miner_iter = get(miner);
					optional<account_object> account_iter = get(miner_iter->miner_account);
					trx_op.miner_address = account_iter->addr;
					trx_op.withdraw_trx = trx_itr->real_transaction.id();
					signed_transaction tx;
					uint32_t expiration_time_offset = 0;
					auto dyn_props = get_dynamic_global_properties();
					operation temp = operation(op);
                                        get_global_properties().parameters.current_fees->set_fee(temp);
                                        tx.set_reference_block(dyn_props.head_block_id);
					tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
					tx.operations.push_back(trx_op);
					tx.validate();
					tx.sign(pk, get_chain_id());
					try {
						push_transaction(tx);
					}FC_CAPTURE_AND_LOG((0));
				}
			}FC_CAPTURE_AND_LOG((0))
		}
	}
}
