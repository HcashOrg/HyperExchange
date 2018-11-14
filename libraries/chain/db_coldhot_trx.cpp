#include <graphene/chain/database.hpp>
#include <graphene/chain/coldhot_transfer_object.hpp>
#include <graphene/chain/eth_seri_record.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_coldhot_transaction(transaction_id_type relate_trx_id, transaction_id_type current_trx_id, signed_transaction current_trx, uint64_t op_type) {
			try {
				if (op_type == operation::tag<coldhot_transfer_operation>::value) {
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
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_sign_trx;
					});
				}
				else if (op_type == operation::tag<coldhot_transfer_combine_sign_operation>::value){
					auto& coldhot_tx_dbs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto coldhot_without_sign_trx_iter = coldhot_tx_dbs.find(relate_trx_id);
					auto coldhot_transfer_trx_iter = coldhot_tx_dbs.find(coldhot_without_sign_trx_iter->relate_trx_id);
					coldhot_trx_state  tx_st;
					auto op = current_trx.operations[0];
					auto combine_op = op.get<coldhot_transfer_combine_sign_operation>();
					std::string asset_symbol = combine_op.asset_symbol;
					if ((asset_symbol.find("ETH") != asset_symbol.npos) || (asset_symbol.find("ERC") != asset_symbol.npos))
					{
						tx_st = coldhot_eth_guard_need_sign;
					}
					else {
						tx_st = coldhot_combine_trx_create;
					}
					modify(*coldhot_transfer_trx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = tx_st;
					});
					modify(*coldhot_without_sign_trx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = tx_st;
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
							obj.curret_trx_state = tx_st;
						});
					}
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.original_trx_id = combine_op.original_trx_id;
						obj.curret_trx_state = tx_st;
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
					auto combine_state_tx_range = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(relate_trx_id);
					for (auto combine_state_tx : boost::make_iterator_range(combine_state_tx_range.first, combine_state_tx_range.second)) {
						if (combine_state_tx.curret_trx_state != coldhot_combine_trx_create && combine_state_tx.curret_trx_state != coldhot_eth_guard_sign) {
							continue;
						}
						auto combine_state_tx_iter = coldhot_db_objs.find(combine_state_tx.current_id);
						modify(*combine_state_tx_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_transaction_confirm;
						});
					}
				}
				else if (op_type == operation::tag<coldhot_cancel_transafer_transaction_operation>::value) {
					auto& coldhot_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto tx_coldhot_original_iter = coldhot_db_objs.find(relate_trx_id);
					modify(*tx_coldhot_original_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_transaction_fail;
					});
				}
				else if (op_type == operation::tag<coldhot_cancel_uncombined_trx_operaion>::value) {
					auto& coldhot_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto range_sign_objs = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(relate_trx_id);
					auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(relate_trx_id);
					auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
					//change without sign trx status
					modify(*tx_coldhot_without_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					//change sign trx status
					for (auto sign_trx_obj : boost::make_iterator_range(range_sign_objs.first,range_sign_objs.second)){
						auto temp_iter = coldhot_db_objs.find(sign_trx_obj.current_id);
						modify(*temp_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_transaction_fail;
						});
					}
					//change coldhot transfer trx status
					modify(*tx_coldhot_original_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_transaction_fail;
					});
				}
				else if (op_type == operation::tag<coldhot_cancel_combined_trx_operaion>::value) {
					auto& coldhot_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto coldhot_combine_trx_iter = coldhot_db_objs.find(relate_trx_id);
					auto range_sign_objs = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(coldhot_combine_trx_iter->relate_trx_id);
					auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(relate_trx_id);
					auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
					modify(*coldhot_combine_trx_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					//change without sign trx status
					modify(*tx_coldhot_without_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					//change sign trx status
					for (auto sign_trx_obj : boost::make_iterator_range(range_sign_objs.first, range_sign_objs.second)) {
						auto temp_iter = coldhot_db_objs.find(sign_trx_obj.current_id);
						modify(*temp_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_transaction_fail;
						});
					}
					//change coldhot transfer trx status
					modify(*tx_coldhot_original_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_transaction_fail;
					});
				}
				else if (op_type == operation::tag<eth_cancel_coldhot_fail_trx_operaion>::value) {
					auto& coldhot_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto eth_final_iter = coldhot_db_objs.find(relate_trx_id);
					auto range_sign_objs = get_index_type<coldhot_transfer_index>().indices().get<by_relate_trx_id>().equal_range(eth_final_iter->relate_trx_id);
					auto tx_coldhot_without_sign_iter = coldhot_db_objs.find(relate_trx_id);
					auto tx_coldhot_original_iter = coldhot_db_objs.find(tx_coldhot_without_sign_iter->relate_trx_id);
					//change without sign trx status
					modify(*tx_coldhot_without_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					modify(*eth_final_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					//change sign trx status
					for (auto sign_trx_obj : boost::make_iterator_range(range_sign_objs.first, range_sign_objs.second)) {
						auto temp_iter = coldhot_db_objs.find(sign_trx_obj.current_id);
						modify(*temp_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_transaction_fail;
						});
					}
					//change coldhot transfer trx status
					modify(*tx_coldhot_original_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_transaction_fail;
					});
					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = eth_final_iter->relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.curret_trx_state = coldhot_transaction_fail;
					});
				}
				else if (op_type == operation::tag<eths_coldhot_guard_sign_final_operation>::value){
				auto & tx_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id> ();
				auto tx_combine_sign_iter = tx_db_objs.find(relate_trx_id);
				auto tx_without_sign_iter = tx_db_objs.find(tx_combine_sign_iter->relate_trx_id);
				auto op = current_trx.operations[0];
				auto eth_signed_op = op.get<eths_coldhot_guard_sign_final_operation>();
				std::string signed_trxid = eth_signed_op.signed_crosschain_trx_id;
				if (eth_signed_op.signed_crosschain_trx_id.find('|') != eth_signed_op.signed_crosschain_trx_id.npos) {
					auto pos = eth_signed_op.signed_crosschain_trx_id.find('|');
					signed_trxid = eth_signed_op.signed_crosschain_trx_id.substr(pos + 1);
				}
				modify(*tx_combine_sign_iter, [&](coldhot_transfer_object& obj) {
					obj.curret_trx_state = coldhot_eth_guard_sign;
					obj.original_trx_id = signed_trxid;
				});
				modify(*tx_without_sign_iter, [&](coldhot_transfer_object& obj) {
					obj.curret_trx_state = coldhot_eth_guard_sign;
					obj.original_trx_id = signed_trxid;
				});
				
				auto tx_user_crosschain_iter = tx_db_objs.find(tx_without_sign_iter->relate_trx_id);
				modify(*tx_user_crosschain_iter, [&](coldhot_transfer_object& obj) {
					obj.curret_trx_state = coldhot_eth_guard_sign;
					obj.original_trx_id = signed_trxid;
				});
				const auto& sign_range = get_index_type< coldhot_transfer_index >().indices().get<by_relate_trx_id>().equal_range(tx_combine_sign_iter->relate_trx_id);
				std::for_each(sign_range.first, sign_range.second, [&](const coldhot_transfer_object& sign_tx) {
					auto sign_iter = tx_db_objs.find(sign_tx.current_id);
					if (sign_iter->curret_trx_state == coldhot_eth_guard_need_sign) {
						modify(*sign_iter, [&](coldhot_transfer_object& obj) {
							obj.curret_trx_state = coldhot_eth_guard_sign;
							obj.original_trx_id = signed_trxid;
						});
					}
				});

				create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
					obj.op_type = op_type;
					obj.relate_trx_id = tx_combine_sign_iter->relate_trx_id;
					obj.current_id = current_trx_id;
					obj.current_trx = current_trx;
					obj.original_trx_id = signed_trxid;
					obj.curret_trx_state = coldhot_eth_guard_sign;
				});
				}
				else if (op_type == operation::tag<eths_guard_coldhot_change_signer_operation>::value) {
					auto & tx_db_objs = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
					auto tx_combine_sign_iter = tx_db_objs.find(relate_trx_id);
					auto tx_without_sign_iter = tx_db_objs.find(tx_combine_sign_iter->relate_trx_id);
					auto op = current_trx.operations[0];
					auto eth_signed_op = op.get<eths_guard_coldhot_change_signer_operation>();
					std::string signed_trxid = eth_signed_op.signed_crosschain_trx_id;
					if (eth_signed_op.signed_crosschain_trx_id.find('|') != eth_signed_op.signed_crosschain_trx_id.npos) {
						auto pos = eth_signed_op.signed_crosschain_trx_id.find('|');
						signed_trxid = eth_signed_op.signed_crosschain_trx_id.substr(pos + 1);
					}
					modify(*tx_combine_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_eth_guard_sign;
						obj.original_trx_id = signed_trxid;
					});
					modify(*tx_without_sign_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_eth_guard_sign;
						obj.original_trx_id = signed_trxid;
					});

					auto tx_user_crosschain_iter = tx_db_objs.find(tx_without_sign_iter->relate_trx_id);
					modify(*tx_user_crosschain_iter, [&](coldhot_transfer_object& obj) {
						obj.curret_trx_state = coldhot_eth_guard_sign;
						obj.original_trx_id = signed_trxid;
					});
					const auto& sign_range = get_index_type< coldhot_transfer_index >().indices().get<by_relate_trx_id>().equal_range(tx_combine_sign_iter->relate_trx_id);
					std::for_each(sign_range.first, sign_range.second, [&](const coldhot_transfer_object& sign_tx) {
						auto sign_iter = tx_db_objs.find(sign_tx.current_id);
						if (sign_iter->curret_trx_state == coldhot_eth_guard_need_sign) {
							modify(*sign_iter, [&](coldhot_transfer_object& obj) {
								obj.curret_trx_state = coldhot_eth_guard_sign;
								obj.original_trx_id = signed_trxid;
							});
						}
					});

					create<coldhot_transfer_object>([&](coldhot_transfer_object& obj) {
						obj.op_type = op_type;
						obj.relate_trx_id = tx_combine_sign_iter->relate_trx_id;
						obj.current_id = current_trx_id;
						obj.current_trx = current_trx;
						obj.original_trx_id = signed_trxid;
						obj.curret_trx_state = coldhot_eth_guard_sign;
					});
				}
			}FC_CAPTURE_AND_RETHROW((relate_trx_id)(current_trx_id))
		}
		void database::create_coldhot_transfer_trx(miner_id_type miner, fc::ecc::private_key pk) {
			//need to check if there are crosschain trx
			auto start = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().lower_bound(withdraw_without_sign_trx_create);
			auto end = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().lower_bound(withdraw_transaction_confirm);
			auto transfer_check1 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_eth_guard_need_sign);
			auto transfer_check2 = get_index_type<crosschain_trx_index>().indices().get<by_transaction_stata>().equal_range(withdraw_eth_guard_sign);
		
			auto coldhot_uncreate_range = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_without_sign_trx_uncreate);
			auto check_point_1 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_without_sign_trx_create);
			auto check_point_2 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_combine_trx_create);
			auto check_point_3 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_eth_guard_need_sign);
			auto check_point_4 = get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_state>().equal_range(coldhot_trx_state::coldhot_eth_guard_sign);
			flat_set<string> asset_set;
			for (auto check_point : boost::make_iterator_range(transfer_check1.first, transfer_check1.second))
			{
				auto op = check_point.real_transaction.operations[0];
				if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>().value)
				{
					auto combineop = op.get<crosschain_withdraw_combine_sign_operation>();
					asset_set.insert(combineop.asset_symbol);
				}
			}
			for (auto check_point : boost::make_iterator_range(transfer_check2.first, transfer_check2.second))
			{
				auto op = check_point.real_transaction.operations[0];
				if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>().value)
				{
					auto combineop = op.get<crosschain_withdraw_combine_sign_operation>();
					asset_set.insert(combineop.asset_symbol);
				}
			}
			for (const auto& check_point : boost::make_iterator_range(check_point_3.first, check_point_3.second))
			{
				auto op = check_point.current_trx.operations[0];
				if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>::value)
				{
					const auto& coldhot_op = op.get<coldhot_transfer_combine_sign_operation>();
					asset_set.insert(coldhot_op.asset_symbol);
				}
			}
			for (const auto& check_point : boost::make_iterator_range(check_point_4.first, check_point_4.second))
			{
				auto op = check_point.current_trx.operations[0];
				if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>::value)
				{
					const auto& coldhot_op = op.get<coldhot_transfer_combine_sign_operation>();
					asset_set.insert(coldhot_op.asset_symbol);
				}
			}
			for (const auto& check_point : boost::make_iterator_range(check_point_1.first, check_point_1.second))
			{
				auto op = check_point.current_trx.operations[0];
				if (op.which() == operation::tag<coldhot_transfer_without_sign_operation>::value)
				{
					const auto& coldhot_op = op.get<coldhot_transfer_without_sign_operation>();
					asset_set.insert(coldhot_op.asset_symbol);
				}
			}

			for (auto check_point : boost::make_iterator_range(check_point_2.first, check_point_2.second))
			{
				auto op = check_point.current_trx.operations[0];
				if (op.which() == operation::tag<coldhot_transfer_with_sign_operation>::value)
				{
					auto coldhot_op = op.get<coldhot_transfer_with_sign_operation>();
					asset_set.insert(coldhot_op.asset_symbol);
				}
				else if (op.which() == operation::tag<coldhot_transfer_combine_sign_operation>::value)
				{
					auto coldhot_op = op.get<coldhot_transfer_combine_sign_operation>();
					asset_set.insert(coldhot_op.asset_symbol);
				}

			}
			while (start != end)
			{
				auto op = start->real_transaction.operations[0];
				if (op.which() == operation::tag<crosschain_withdraw_without_sign_operation>::value)
				{
					auto cross_op = op.get<crosschain_withdraw_without_sign_operation>();
					asset_set.insert(cross_op.asset_symbol);
				}
				else if (op.which() == operation::tag<crosschain_withdraw_with_sign_operation>::value)
				{
					auto cross_op = op.get<crosschain_withdraw_with_sign_operation>();
					asset_set.insert(cross_op.asset_symbol);
				}
				else if (op.which() == operation::tag<crosschain_withdraw_combine_sign_operation>::value)
				{
					auto cross_op = op.get<crosschain_withdraw_combine_sign_operation>();
					asset_set.insert(cross_op.asset_symbol);
				}
				start++;
			}


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
					if (asset_set.find(coldhot_op.asset_symbol) != asset_set.end())
						continue;
					auto& multi_account_pair_hot_db = get_index_type<multisig_account_pair_index>().indices().get<by_bindhot_chain_type>();
					auto& multi_account_pair_cold_db = get_index_type<multisig_account_pair_index>().indices().get<by_bindcold_chain_type>();
					multisig_account_pair_id_type withdraw_multi_id;
					auto hot_iter  = multi_account_pair_hot_db.find(boost::make_tuple(coldhot_op.multi_account_withdraw, coldhot_op.asset_symbol));
					auto cold_iter = multi_account_pair_cold_db.find(boost::make_tuple(coldhot_op.multi_account_withdraw, coldhot_op.asset_symbol));
					if (hot_iter != multi_account_pair_hot_db.end()){
						withdraw_multi_id = hot_iter->id;
					}
					else if (cold_iter != multi_account_pair_cold_db.end()){
						withdraw_multi_id = cold_iter->id;
					}
					const auto & multi_range = get_index_type<multisig_address_index>().indices().get<by_multisig_account_pair_id>().equal_range(withdraw_multi_id);
					int withdraw_account_count = 0;
					for (auto multi_account : boost::make_iterator_range(multi_range.first,multi_range.second)){
						++withdraw_account_count;
					}
					if (!manager.contain_crosschain_handles(coldhot_op.asset_symbol))
						continue;
					auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
					coldhot_transfer_without_sign_operation trx_op;
					std::map<string, string> dest_info;
					dest_info[std::string(coldhot_op.multi_account_deposit)] = coldhot_op.amount;
					if (coldhot_op.asset_symbol == "ETH" || coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos)
					{
						auto & asset_db = get_index_type<asset_index>().indices().get<by_symbol>();
						auto asset_iter = asset_db.find(coldhot_op.asset_symbol);
						FC_ASSERT(asset_iter != asset_db.end());

						auto block_num = get_dynamic_global_properties().head_block_number;
						std::string temp_memo = fc::to_string(block_num);
						std::string erc_and_precision = asset_iter->options.description;
						if ((coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos) && erc_and_precision.find('|') != erc_and_precision.npos) {
							temp_memo += '|' + erc_and_precision;
						}
						trx_op.coldhot_trx_original_chain = crosschain_plugin->create_multisig_transaction(std::string(coldhot_op.multi_account_withdraw), dest_info, coldhot_op.asset_symbol, temp_memo);
					}
					else {
						trx_op.coldhot_trx_original_chain = crosschain_plugin->create_multisig_transaction(std::string(coldhot_op.multi_account_withdraw),
							dest_info,
							coldhot_op.asset_symbol,
							coldhot_op.memo);
					}
					trx_op.withdraw_account_count = withdraw_account_count;
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
					auto & tx_relate_db = get_index_type<coldhot_transfer_index>().indices().get<by_current_trxidstate>();
					const auto& guard_db = get_index_type<guard_member_index>().indices().get<by_id>();
					auto relate_tx_iter = tx_relate_db.find(boost::make_tuple(trxs.first, coldhot_without_sign_trx_create));
					if (relate_tx_iter == tx_relate_db.end() || relate_tx_iter->current_trx.operations.size() < 1) {
						continue;
					}
					auto op = relate_tx_iter->current_trx.operations[0];
					auto coldhot_op = op.get<coldhot_transfer_without_sign_operation>();
					if (trxs.second.size() < (coldhot_op.withdraw_account_count*2/3 + 1)) {
						continue;
					}
					account_id_type sign_senator;
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
						if (sign_senator == account_id_type()) {
							auto guard_iter = guard_db.find(coldhot_sign_op.sign_guard);
							FC_ASSERT(guard_iter != guard_db.end());
							if (guard_iter->senator_type == PERMANENT) {
								sign_senator = guard_iter->guard_member_account;
							}
						}
					}
					if (transaction_err) {
						continue;
					}
					auto& manager = graphene::crosschain::crosschain_manager::get_instance();
					if (!manager.contain_crosschain_handles(coldhot_op.asset_symbol))
						continue;
					auto crosschain_plugin = manager.get_crosschain_handle(coldhot_op.asset_symbol);
					coldhot_transfer_combine_sign_operation trx_op;

					vector<string> guard_signs(combine_signature.begin(), combine_signature.end());
					if (coldhot_op.asset_symbol == "ETH" || coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos)
					{
						try {
							auto guard_account_id = sign_senator;
							multisig_address_object senator_multi_obj;
							const auto& multisig_addr_by_guard = get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
							const auto iter_range = multisig_addr_by_guard.equal_range(boost::make_tuple(coldhot_op.asset_symbol, guard_account_id));
							const auto& multi_account_db = get_index_type<multisig_account_pair_index>().indices().get<by_id>();
							std::map<multisig_account_pair_id_type, string> mulsig_hot;
							std::map<multisig_account_pair_id_type, string> mulsig_cold;
							for (auto item : boost::make_iterator_range(iter_range.first, iter_range.second))
							{
								mulsig_hot[item.multisig_account_pair_object_id] = item.new_address_hot;
								mulsig_cold[item.multisig_account_pair_object_id] = item.new_address_cold;
							}
							std::string contrat_address = coldhot_op.coldhot_trx_original_chain["contract_addr"].as_string();
							std::string address_to_sign_eth_trx = "";
							for (auto multi_acc : mulsig_hot) {
								auto multi_account_temp = multi_account_db.find(multi_acc.first);
								FC_ASSERT(multi_account_temp != multi_account_db.end());
								if (multi_account_temp->bind_account_hot == contrat_address)
								{
									address_to_sign_eth_trx = multi_acc.second;
									break;
								}
							}
							for (auto multi_acc : mulsig_cold) {
								auto multi_account_temp = multi_account_db.find(multi_acc.first);
								FC_ASSERT(multi_account_temp != multi_account_db.end());
								if (multi_account_temp->bind_account_cold == contrat_address)
								{
									address_to_sign_eth_trx = multi_acc.second;
									break;
								}
							}
							FC_ASSERT(address_to_sign_eth_trx != "");
							fc::mutable_variant_object multi_obj;
							multi_obj.set("signer", address_to_sign_eth_trx);
							multi_obj.set("source_trx", coldhot_op.coldhot_trx_original_chain);
							auto temp_obj = fc::variant_object(multi_obj);
							trx_op.coldhot_trx_original_chain = crosschain_plugin->merge_multisig_transaction(temp_obj, guard_signs);
						}	FC_CAPTURE_AND_LOG((0));
					}
					else {
						trx_op.coldhot_trx_original_chain = crosschain_plugin->merge_multisig_transaction(coldhot_op.coldhot_trx_original_chain, guard_signs);
					}
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
					if (coldhot_op.asset_symbol == "ETH" || coldhot_op.asset_symbol.find("ERC") != coldhot_op.asset_symbol.npos) {
						auto source_without_sign_trx = trx_op.coldhot_trx_original_chain;
						if ((source_without_sign_trx.contains("msg_prefix")) && (source_without_sign_trx.contains("contract_addr"))) {
							trx_op.original_trx_id = source_without_sign_trx["contract_addr"].as_string() + '|' + source_without_sign_trx["msg_prefix"].as_string();
						}
					}
					else {
						trx_op.original_trx_id = hdl_trx.trxs.begin()->second.trx_id;
					}
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
