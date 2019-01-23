#include <graphene/chain/database.hpp>
#include <fc/string.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
#include <fc/thread/scoped_lock.hpp>
#include <graphene/chain/committee_member_object.hpp>

namespace graphene {
	namespace chain {
		void database::adjust_deposit_to_link_trx(const hd_trx& handled_trx) {
			try {
				fc::scoped_lock<std::mutex> _lock(db_lock);
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
				fc::scoped_lock<std::mutex> _lock(db_lock);
				auto & deposit_db = get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				string crosschain_trx_id = handled_trx.trx_id;
				if (handled_trx.asset_symbol == "ETH" || handled_trx.asset_symbol.find("ERC") != handled_trx.asset_symbol.npos)
				{
					if (handled_trx.trx_id.find('|') != handled_trx.trx_id.npos)
					{
						auto pos = handled_trx.trx_id.find('|');
						crosschain_trx_id = handled_trx.trx_id.substr(pos + 1);
					}
				}
				auto deposit_to_link_trx = deposit_db.find(crosschain_trx_id);
				if (deposit_to_link_trx == deposit_db.end()) {
					create<acquired_crosschain_trx_object>([&](acquired_crosschain_trx_object& obj) {
						obj.handle_trx = handled_trx;
						obj.handle_trx_id = crosschain_trx_id;
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
						obj.block_num = head_block_num();
						obj.symbol = withdraw_op.asset_symbol;
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
						obj.block_num = head_block_num();
					});
					if (trx_without_sign_trx_iter == trx_db.end())
					{
						create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
							obj.op_type = op_type;
							obj.relate_transaction_id = relate_transaction_id;
							obj.real_transaction = real_transaction;
							obj.transaction_id = transaction_id;
							obj.all_related_origin_transaction_ids = relate_transaction_ids;
							obj.block_num = head_block_num();
							obj.symbol = trx_crosschain_withdraw_iter->symbol;
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
					transaction_stata trx_st;
					std::string asset_symbol = combine_op.asset_symbol;
					if ((asset_symbol=="ETH")|| (asset_symbol.find("ERC") != asset_symbol.npos))
					{
						trx_st = withdraw_eth_guard_need_sign;
					}
					else {
						trx_st = withdraw_combine_trx_create;
					}
					modify(*tx_without_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = trx_st;
						obj.block_num = head_block_num();
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = trx_st;
							obj.crosschain_trx_id = combine_op.crosschain_trx_id;
							obj.block_num = head_block_num();
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
								obj.trx_state = trx_st;
								obj.block_num = head_block_num();
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
						obj.trx_state = trx_st;
						obj.block_num = head_block_num();
						obj.symbol = combine_op.asset_symbol;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_with_sign_evaluate::operation_type>::value) {
					auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_itr_relate = trx_db_relate.find(relate_transaction_id);
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
						obj.block_num = head_block_num();
						obj.symbol = trx_itr_relate->symbol;
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
						obj.block_num = head_block_num();
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = withdraw_transaction_confirm;
							obj.block_num = head_block_num();
						});
					}

					const auto & combine_state_range = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>().equal_range(relate_transaction_id );
					int count = 0;
					std::for_each(combine_state_range.first, combine_state_range.second, [&](const crosschain_trx_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						count++;
						if (sign_iter->trx_state == withdraw_combine_trx_create || sign_iter->trx_state == withdraw_eth_guard_sign) {
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_transaction_confirm;
								obj.block_num = head_block_num();
							});
						}
					});
				}
				else if (op_type == operation::tag<eths_guard_sign_final_operation>::value) {
					auto & tx_db_objs = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto tx_combine_sign_iter = tx_db_objs.find(relate_transaction_id);
					auto tx_without_sign_iter = tx_db_objs.find(tx_combine_sign_iter->relate_transaction_id);
					auto op = real_transaction.operations[0];
					auto eth_signed_op = op.get<eths_guard_sign_final_operation>();
					std::string signed_trxid = eth_signed_op.signed_crosschain_trx_id;
					if (eth_signed_op.signed_crosschain_trx_id.find('|') != eth_signed_op.signed_crosschain_trx_id.npos) {
						auto pos = eth_signed_op.signed_crosschain_trx_id.find('|');
						signed_trxid = eth_signed_op.signed_crosschain_trx_id.substr(pos + 1);
					}
					
					modify(*tx_combine_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_eth_guard_sign;
						obj.crosschain_trx_id = signed_trxid;
						obj.block_num = head_block_num();
					});
					modify(*tx_without_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_eth_guard_sign;
						obj.block_num = head_block_num();
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = withdraw_eth_guard_sign;
							obj.crosschain_trx_id = signed_trxid;
							obj.block_num = head_block_num();
						});
					}
					const auto& sign_range = get_index_type< crosschain_trx_index >().indices().get<by_relate_trx_id>().equal_range(tx_combine_sign_iter->relate_transaction_id);
					int count = 0;
					std::for_each(sign_range.first, sign_range.second, [&](const crosschain_trx_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						count++;
						if (sign_iter->trx_state == withdraw_eth_guard_need_sign) {
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_eth_guard_sign;
								obj.crosschain_trx_id = signed_trxid;
								obj.block_num = head_block_num();
							});
						}
					});

					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = tx_combine_sign_iter->relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.crosschain_trx_id = signed_trxid;
						obj.trx_state = withdraw_eth_guard_sign;
						obj.block_num = head_block_num();
						obj.symbol = eth_signed_op.chain_type;
					});
				}
				else if (op_type == operation::tag<eths_guard_change_signer_operation>::value) {
					auto & tx_db_objs = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto tx_combine_sign_iter = tx_db_objs.find(relate_transaction_id);
					auto tx_without_sign_iter = tx_db_objs.find(tx_combine_sign_iter->relate_transaction_id);
					auto op = real_transaction.operations[0];
					auto eth_signed_op = op.get<eths_guard_change_signer_operation>();
					std::string signed_trxid = eth_signed_op.signed_crosschain_trx_id;
					if (eth_signed_op.signed_crosschain_trx_id.find('|') != eth_signed_op.signed_crosschain_trx_id.npos) {
						auto pos = eth_signed_op.signed_crosschain_trx_id.find('|');
						signed_trxid = eth_signed_op.signed_crosschain_trx_id.substr(pos + 1);
					}
					modify(*tx_combine_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_eth_guard_sign;
						obj.crosschain_trx_id = signed_trxid;
						obj.block_num = head_block_num();
					});
					modify(*tx_without_sign_iter, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_eth_guard_sign;
						obj.block_num = head_block_num();
					});
					for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
						auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
						modify(*tx_user_crosschain_iter, [&](crosschain_trx_object& obj) {
							obj.trx_state = withdraw_eth_guard_sign;
							obj.crosschain_trx_id = signed_trxid;
							obj.block_num = head_block_num();
						});
					}
					const auto& sign_range = get_index_type< crosschain_trx_index >().indices().get<by_relate_trx_id>().equal_range(tx_combine_sign_iter->relate_transaction_id);
					int count = 0;
					std::for_each(sign_range.first, sign_range.second, [&](const crosschain_trx_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.transaction_id);
						count++;
						if (sign_iter->trx_state == withdraw_eth_guard_need_sign) {
							modify(*sign_iter, [&](crosschain_trx_object& obj) {
								obj.trx_state = withdraw_eth_guard_sign;
								obj.crosschain_trx_id = signed_trxid;
								obj.block_num = head_block_num();
							});
						}
					});

					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = tx_combine_sign_iter->relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.crosschain_trx_id = signed_trxid;
						obj.trx_state = withdraw_eth_guard_sign;
						obj.block_num = head_block_num();
						obj.symbol = eth_signed_op.chain_type;
					});
				}
			}FC_CAPTURE_AND_RETHROW((relate_transaction_id)(transaction_id))
		}
		void create_eth_call_message_param(){

		}
		void database::create_result_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			try {
				//need to check if multisig transaction created
				auto start = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().lower_bound(coldhot_trx_state::coldhot_without_sign_trx_create);
				auto end = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().lower_bound(coldhot_trx_state::coldhot_transaction_confirm);
				auto coldhot_check1 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_eth_guard_need_sign);
				auto coldhot_check2 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_eth_guard_sign);
				//need to check if can be created
				auto check_point_1 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_without_sign_trx_create);
				auto check_point_2 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_combine_trx_create);
				auto check_point_3 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_eth_guard_need_sign);
				auto check_point_4 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_eth_guard_sign);
				flat_set<string> asset_set;
				for (auto check_point : boost::make_iterator_range(coldhot_check1.first, coldhot_check1.second))
				{
					auto op = check_point.current_trx.operations[0];
					if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>().value)
					{
						auto withop = op.get<coldhot_transfer_combine_sign_operation>();
						asset_set.insert(withop.asset_symbol);
					}
				}
				for (auto check_point : boost::make_iterator_range(coldhot_check2.first, coldhot_check2.second))
				{
					auto op = check_point.current_trx.operations[0];
					if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>().value)
					{
						auto withop = op.get<coldhot_transfer_combine_sign_operation>();
						asset_set.insert(withop.asset_symbol);
					}
				}
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
					else if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>().value)
					{
						auto withop = op.get<crosschain_withdraw_combine_sign_operation>();
						asset_set.insert(withop.asset_symbol);
					}

				}
				for (auto check_point : boost::make_iterator_range(check_point_3.first, check_point_3.second))
				{
					auto op = check_point.real_transaction.operations[0];
					if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>().value)
					{
						auto combineop = op.get<crosschain_withdraw_combine_sign_operation>();
						asset_set.insert(combineop.asset_symbol);
					}
				}
				for (auto check_point : boost::make_iterator_range(check_point_4.first, check_point_4.second))
				{
					auto op = check_point.real_transaction.operations[0];
					if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>().value)
					{
						auto combineop = op.get<crosschain_withdraw_combine_sign_operation>();
						asset_set.insert(combineop.asset_symbol);
					}
				}
				while (start != end)
				{
					auto op = start->current_trx.operations[0];
					if (op.which() == operation::tag<coldhot_transfer_without_sign_operation>::value)
					{
						auto cross_op = op.get<coldhot_transfer_without_sign_operation>();
						asset_set.insert(cross_op.asset_symbol);
					}
					else if (op.which() == operation::tag<coldhot_transfer_with_sign_operation>::value)
					{
						auto cross_op = op.get<coldhot_transfer_with_sign_operation>();
						asset_set.insert(cross_op.asset_symbol);
					}
					else if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>::value)
					{
						auto cross_op = op.get<coldhot_transfer_combine_sign_operation>();
						asset_set.insert(cross_op.asset_symbol);
					}
					start++;
				}
				auto withdraw_range = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_without_sign_trx_uncreate);
				std::map<asset_id_type, std::map<std::string, std::string>> dest_info;
				std::map<std::string, std::string> memo_info;
				std::map<std::string, vector<transaction_id_type>> ccw_trx_ids;
				map<asset_id_type, double> fee;
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				for (auto & cross_chain_trx : boost::make_iterator_range(withdraw_range.first,withdraw_range.second)) {
					if (dest_info.size() >= 10)
						break;
					if (cross_chain_trx.real_transaction.id() != cross_chain_trx.transaction_id || cross_chain_trx.real_transaction.operations.size() < 1) {
						continue;
					}
					auto op = cross_chain_trx.real_transaction.operations[0];
					auto withop = op.get<crosschain_withdraw_evaluate::operation_type>();
					if (withop.asset_symbol == "ETH"|| withop.asset_symbol.find("ERC") != withop.asset_symbol.npos){
						if (dest_info[withop.asset_id].size() > 20){
							continue;
						}
					}
					else {
						if (dest_info[withop.asset_id].size() > 20) {
							continue;
						}
					}
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
					if (!manager.contain_crosschain_handles(std::string(withop.asset_symbol)))
						continue;
					auto hdl = manager.get_crosschain_handle(std::string(withop.asset_symbol));
					if (hdl == nullptr)
						continue;
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
						temp_map[withop.crosschain_account] = fc::to_string(fc::to_double(temp_map[withop.crosschain_account]) + temp_amount).c_str();
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
					if (hdl == nullptr)
						continue;
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
					char temp_amount[1024];
					for (auto& item : one_asset.second)
					{
						memset(temp_amount, 0, 1024);
						std::sprintf(temp_amount, format.c_str(), fc::to_double(item.second));
						item.second = temp_amount;
					}
					crosschain_trx hdtrxs;
					if (asset_symbol == "ETH" || asset_symbol.find("ERC") != asset_symbol.npos)
					{
						auto & asset_db = get_index_type<asset_index>().indices().get<by_symbol>();
						auto asset_iter = asset_db.find(asset_symbol);
						FC_ASSERT(asset_iter != asset_db.end());
						
						auto block_num = get_dynamic_global_properties().head_block_number;
						std::string temp_memo = fc::to_string(block_num);
						std::string erc_and_precision = asset_iter->options.description;
						if ((asset_symbol.find("ERC") != asset_symbol.npos) && erc_and_precision.find('|') != erc_and_precision.npos) {
							temp_memo += '|' + erc_and_precision;
						}						
						trx_op.withdraw_source_trx = hdl->create_multisig_transaction(multi_account_obj.bind_account_hot, one_asset.second, asset_symbol, temp_memo);
						hdtrxs = hdl->turn_trxs(fc::variant_object("turn_without_eth_sign",trx_op.withdraw_source_trx));
					}
					else {
						trx_op.withdraw_source_trx = hdl->create_multisig_transaction(multi_account_obj.bind_account_hot, one_asset.second, asset_symbol, memo_info[asset_symbol]);
						hdtrxs = hdl->turn_trxs(trx_op.withdraw_source_trx);
					}
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
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				get_index_type<acquired_crosschain_index>().inspect_all_objects([&](const object& o) {
					const acquired_crosschain_trx_object& p = static_cast<const acquired_crosschain_trx_object&>(o);
					if (p.acquired_transaction_state == acquired_trx_uncreate && manager.contain_crosschain_handles(p.handle_trx.asset_symbol)){
						if (head_block_num() >= DB_CROSSCHAIN_TRX_220000)
						{
							acquired_crosschain_trx[p.handle_trx.asset_symbol].push_back(p);
						}
						else if (p.handle_trx.trx_id != "e5a5043b7f4ae26793069b535de1ad0a443a6ae48c733b44aaad9662969fc960" &&
							p.handle_trx.trx_id != "9b81ad5f2706bac7bcb4f1021ee4bc0cbcd44e46ed7da858c3a0a1fbfe9a901d" &&
							p.handle_trx.trx_id != "3eb700c63e200787d7ee9618e68564301516785347a9916382050234a6ec8711" &&
							p.handle_trx.trx_id != "a72207de343b0b768373ac4a3e8313b30f4383468bf705010e60175ee9951495" &&
							p.handle_trx.trx_id != "caec0a90a1eb720c650379371981abf708101cc836cd9253ce3323796c50df72" &&
							p.handle_trx.trx_id != "c18e2aab16c46e28f1f7f6a16dad370cf163138d9ff565003363048be933d2f4" &&
							p.handle_trx.trx_id != "f10485e8e51be5d52980186831103f685ab61d40c2f681f4d8128995c0a42998" &&
							p.handle_trx.trx_id != "364fb6d4bb4067f374922ffb52f12794afb73998d06dce89807a2eb535d8e116" &&
							p.handle_trx.trx_id != "cf4b6b40d0cc961266571a536d4c09a2e259a668dbc578cf4c9457be886ca96e" &&
							p.handle_trx.trx_id != "1f46b81cf37a94ee8caa89ea07af459ca7b1f563b09c98d86bd79867e0531e4b" &&
							p.handle_trx.trx_id != "19d2186de845ce5b3dcbe066cf18a8540bea4a204d66c0f96ad0e723aea673d9" &&
							p.handle_trx.trx_id != "2048d25667f95babc0ec7985d439c7ca03c0ac4dd1cc64ff50f893e963b14d97" &&
							p.handle_trx.trx_id != "fcc47b568267a4ad63785ba899e0f426e8bd9729df159e007e32488d160324de" &&
							p.handle_trx.trx_id != "b758ba6cf67e3697c01479b33fbe133d7b497109947cc448b62c67ea779a9616" &&
							p.handle_trx.trx_id != "a666d28912419d2a82ef7c3a222a3e99dd72e4bda5d56246cc368dfd21cbd7f8" &&
							p.handle_trx.trx_id != "4cdb41c2cc4a3cb2e56cd2febacc00327cb07c60353f09f0d637f81d28febd02")
						{
							acquired_crosschain_trx[p.handle_trx.asset_symbol].push_back(p);
						}
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
						if ((acquired_trxs.first.find("ETH") != acquired_trxs.first.npos) || (acquired_trxs.first.find("ERC") != acquired_trxs.first.npos)) {
							const auto cold_range = get_index_type<eth_multi_account_trx_index>().indices().get<by_eth_cold_multi_trx_id>().equal_range(acquired_trx.handle_trx_id);
							const auto hot_range = get_index_type<eth_multi_account_trx_index>().indices().get<by_eth_hot_multi_trx_id>().equal_range(acquired_trx.handle_trx_id);
							bool eth_cold_account_create_trx = false;
							bool eth_hot_account_create_trx = false;
							transaction_id_type pre_trx_id;
							for (auto cold_multi_obj : boost::make_iterator_range(cold_range.first, cold_range.second))
							{
								if (cold_multi_obj.state == sol_create_guard_signed 
									&& cold_multi_obj.op_type == operation::tag<eths_multi_sol_guard_sign_operation>::value
									&& cold_multi_obj.cold_trx_success == false) {
									eth_cold_account_create_trx = true;
									pre_trx_id = cold_multi_obj.multi_account_create_trx_id;
									break;
								}
							}
							for (auto hot_multi_obj : boost::make_iterator_range(hot_range.first, hot_range.second))
							{
								if (hot_multi_obj.state == sol_create_guard_signed 
									&& hot_multi_obj.op_type == operation::tag<eths_multi_sol_guard_sign_operation>::value
									&& hot_multi_obj.hot_trx_success == false) {
									eth_hot_account_create_trx = true;
									pre_trx_id = hot_multi_obj.multi_account_create_trx_id;
									break;
								}
							}
							if (eth_cold_account_create_trx || eth_hot_account_create_trx) {
								eth_multi_account_create_record_operation op;
								op.miner_broadcast = miner;
								optional<miner_object> miner_iter = get(miner);
								optional<account_object> account_iter = get(miner_iter->miner_account);
								op.miner_address = account_iter->addr;
								op.chain_type = acquired_trx.handle_trx.asset_symbol;
								op.eth_multi_account_trx = acquired_trx.handle_trx;
								op.pre_trx_id = pre_trx_id;
								if (eth_cold_account_create_trx)
								{
									op.multi_pubkey_type = "cold";
								}
								else {
									op.multi_pubkey_type = "hot";
								}
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
							}
							else {
								std::string to_account;
								auto pos = acquired_trx.handle_trx.to_account.find("|");
								if (pos != acquired_trx.handle_trx.to_account.npos) {
									to_account = acquired_trx.handle_trx.to_account.substr(0, pos);
								}
								else {
									to_account = acquired_trx.handle_trx.to_account;
								}
								bool multi_account_withdraw_cold = false;
								bool multi_account_deposit_cold = false;
								bool multi_account_withdraw_hot = false;
								bool multi_account_deposit_hot = false;
								auto to_iter_cold = multi_account_obj_cold.find(to_account);
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
								auto to_iter_hot = multi_account_obj_hot.find(to_account);
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
									const auto &tunnel_idx = get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
									const auto tunnel_itr = tunnel_idx.find(boost::make_tuple(acquired_trx.handle_trx.from_account, acquired_trx.handle_trx.asset_symbol));
									if (tunnel_itr == tunnel_idx.end())
										continue;
									op.asset_id = asset_itr->id;
									op.asset_symbol = acquired_trx.handle_trx.asset_symbol;
									op.cross_chain_trx = acquired_trx.handle_trx;
									op.miner_broadcast = miner;
									optional<miner_object> miner_iter = get(miner);
									optional<account_object> account_iter = get(miner_iter->miner_account);
									op.miner_address = account_iter->addr;
									op.deposit_address = tunnel_itr->owner;
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
									}
									catch (fc::exception&)
									{
										fc::scoped_lock<std::mutex> _lock(db_lock);
										acquired_crosschain_trx_object& acq_obj = *(acquired_crosschain_trx_object*)&get_object(acquired_trx.id);
										modify(acq_obj, [&](acquired_crosschain_trx_object& obj) {
											obj.acquired_transaction_state = acquired_trx_deposit_failure;
										});
									}
									continue;
								}
								else if (multi_account_withdraw_hot) {
									crosschain_withdraw_result_operation op;
									auto& originaldb = get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
									auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
									string crosschain_trx_id = acquired_trx.handle_trx.trx_id;
									if (acquired_trx.handle_trx.asset_symbol == "ETH" || acquired_trx.handle_trx.asset_symbol.find("ERC") != acquired_trx.handle_trx.asset_symbol.npos)
									{
										if (acquired_trx.handle_trx.trx_id.find('|') != acquired_trx.handle_trx.trx_id.npos)
										{
											auto pos = acquired_trx.handle_trx.trx_id.find('|');
											crosschain_trx_id = acquired_trx.handle_trx.trx_id.substr(pos + 1);
										}
									}
									auto combine_trx_iter = originaldb.find(boost::make_tuple(crosschain_trx_id, combine_op_number));
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
							continue;
						}
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
							const auto &tunnel_idx = get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
							const auto tunnel_itr = tunnel_idx.find(boost::make_tuple(acquired_trx.handle_trx.from_account, acquired_trx.handle_trx.asset_symbol));
							if (tunnel_itr == tunnel_idx.end())
								continue;
							op.asset_id = asset_itr->id;
							op.asset_symbol = acquired_trx.handle_trx.asset_symbol;
							op.cross_chain_trx = acquired_trx.handle_trx;
							op.miner_broadcast = miner;
							optional<miner_object> miner_iter = get(miner);
							optional<account_object> account_iter = get(miner_iter->miner_account);
							op.miner_address = account_iter->addr;
							op.deposit_address = tunnel_itr->owner;
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
							}
							catch (fc::exception&)
							{
								fc::scoped_lock<std::mutex> _lock(db_lock);
								acquired_crosschain_trx_object& acq_obj = *(acquired_crosschain_trx_object*)&get_object(acquired_trx.id);
								modify(acq_obj,[&](acquired_crosschain_trx_object& obj) {
									obj.acquired_transaction_state = acquired_trx_deposit_failure;
								});
							}
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
				auto guard_members = get_guard_members();
				for (auto & trxs : uncombine_trxs_counts) {
					//TODO : Use macro instead this magic number
					if (trxs.second.size() < (guard_members.size()*2/3+1)) {
						continue;
					}
					map<guard_member_id_type,string> combine_signature;
					account_id_type sign_senator;
					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					const auto& guard_db = get_index_type<guard_member_index>().indices().get<by_id>();
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
						combine_signature[with_sign_op.sign_guard]= with_sign_op.ccw_trx_signature;
						if (sign_senator == account_id_type()) {
							auto guard_iter = guard_db.find(with_sign_op.sign_guard);
							FC_ASSERT(guard_iter != guard_db.end());
							if (guard_iter->senator_type == PERMANENT) {
								sign_senator = guard_iter->guard_member_account;
							}
							
						}
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
					if (!manager.contain_crosschain_handles(std::string(with_sign_op.asset_symbol)))
						continue;
					auto hdl = manager.get_crosschain_handle(std::string(with_sign_op.asset_symbol));
					crosschain_withdraw_combine_sign_operation trx_op;
					vector<string> guard_signed;
					for (const auto& iter : combine_signature)
					{
						guard_signed.push_back(iter.second);
						if (guard_signed.size() == (guard_members.size() * 2 / 3 + 1))
							break;
					}
					if (with_sign_op.asset_symbol == "ETH" || with_sign_op.asset_symbol.find("ERC") != with_sign_op.asset_symbol.npos)
					{
						try {
							auto guard_account_id =sign_senator;
							multisig_address_object senator_multi_obj;
							const auto& multisig_addr_by_guard = get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
							const auto iter_range = multisig_addr_by_guard.equal_range(boost::make_tuple(with_sign_op.asset_symbol, guard_account_id));
							const auto& multi_account_db = get_index_type<multisig_account_pair_index>().indices().get<by_id>();
							std::map<multisig_account_pair_id_type, string> mulsig;
							for (auto item : boost::make_iterator_range(iter_range.first,iter_range.second))
							{
								mulsig[item.multisig_account_pair_object_id] = item.new_address_hot;
							}
							std::string contrat_address = with_sign_op.withdraw_source_trx["contract_addr"].as_string();
							std::string address_to_sign_eth_trx = "";
							for (auto multi_acc : mulsig) {
								auto multi_account_temp = multi_account_db.find(multi_acc.first);
								FC_ASSERT(multi_account_temp != multi_account_db.end());
								if (multi_account_temp->bind_account_hot == contrat_address)
								{
									address_to_sign_eth_trx = multi_acc.second;
									break;
								}								
							}
							std::string gas_price = "5000000000";
							try{
								const auto& asset_indx = get_index_type<asset_index>().indices().get<by_symbol>();
								const auto asset_iter = asset_indx.find(with_sign_op.asset_symbol);
								FC_ASSERT(asset_iter != asset_indx.end());
								gas_price = asset_iter->dynamic_data(*this).gas_price;
							}
							catch (...) {

							}
							FC_ASSERT(address_to_sign_eth_trx != "");
							fc::mutable_variant_object multi_obj;
							multi_obj.set("signer", address_to_sign_eth_trx);
							multi_obj.set("source_trx", with_sign_op.withdraw_source_trx);
							multi_obj.set("gas_price", gas_price);
							auto temp_obj = fc::variant_object(multi_obj);
							trx_op.cross_chain_trx = hdl->merge_multisig_transaction(temp_obj, guard_signed);
						}	FC_CAPTURE_AND_LOG((0));					
					}
					else {
					trx_op.cross_chain_trx = hdl->merge_multisig_transaction(with_sign_op.withdraw_source_trx, guard_signed);
					}
					
					trx_op.asset_symbol = with_sign_op.asset_symbol;
					trx_op.signed_trx_ids.swap(trxs.second);
					trx_op.crosschain_fee = with_sign_op.crosschain_fee;
					trx_op.miner_broadcast = miner;
					if (with_sign_op.asset_symbol == "ETH" || with_sign_op.asset_symbol.find("ERC") != with_sign_op.asset_symbol.npos){
						auto source_without_sign_trx = with_sign_op.withdraw_source_trx;
						if ((source_without_sign_trx.contains("msg_prefix")) && (source_without_sign_trx.contains("contract_addr"))) {
							trx_op.crosschain_trx_id = source_without_sign_trx["contract_addr"].as_string() + '|' + source_without_sign_trx["msg_prefix"].as_string();
						}
							
					}
					else {
					trx_op.crosschain_trx_id = hdl->turn_trxs(trx_op.cross_chain_trx).trxs.begin()->second.trx_id;
					}
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
