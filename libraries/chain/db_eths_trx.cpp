#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_eths_multi_account_record(transaction_id_type pre_trx_id, transaction_id_type current_trx_id, signed_transaction current_trx, uint64_t op_type){
			try{
				if (op_type == operation::tag<eth_series_multi_sol_create_operation>::value){
					auto op = current_trx.operations[0].get<eth_series_multi_sol_create_operation>();
					create<eth_multi_account_trx_object>([&](eth_multi_account_trx_object& obj) {
						obj.multi_account_create_trx_id = current_trx_id;
						obj.object_transaction = current_trx;
						obj.op_type = op_type;
						obj.hot_addresses = op.multi_hot_address;
						obj.cold_addresses = op.multi_cold_address;
						obj.multi_account_pre_trx_id = pre_trx_id;
						obj.cold_trx_success = false;
						obj.symbol = op.chain_type;
						obj.hot_trx_success = false;
						obj.state = sol_create_need_guard_sign;
					});
				}
				else if (op_type == operation::tag<eths_multi_sol_guard_sign_operation>::value) {
					const auto& multi_account_create_db =get_index_type<eth_multi_account_trx_index>().indices().get<by_mulaccount_trx_id>();
					auto op = current_trx.operations[0].get<eths_multi_sol_guard_sign_operation>();
					auto multi_account_create_iter = multi_account_create_db.find(pre_trx_id);
					modify(*multi_account_create_iter, [&](eth_multi_account_trx_object& obj) {
						obj.state = sol_create_guard_signed;
						obj.cold_sol_trx_id = op.multi_cold_trxid;
						obj.hot_sol_trx_id = op.multi_hot_trxid;
					});
					create<eth_multi_account_trx_object>([&](eth_multi_account_trx_object& obj) {
						obj.multi_account_create_trx_id = current_trx_id;
						obj.multi_account_pre_trx_id = pre_trx_id;
						obj.object_transaction = current_trx;
						obj.op_type = op_type;
						obj.cold_sol_trx_id = op.multi_cold_trxid;
						obj.cold_trx_success = false;
						obj.symbol = op.chain_type;
						obj.hot_trx_success = false;
						obj.hot_sol_trx_id = op.multi_hot_trxid;
						obj.state = sol_create_guard_signed;
					});
				}
				else if (op_type == operation::tag<eth_multi_account_create_record_operation>::value) {
					const auto& multi_account_create_db = get_index_type<eth_multi_account_trx_index>().indices().get<by_mulaccount_trx_id>();
					auto multi_account_withsign_iter = multi_account_create_db.find(pre_trx_id);
					auto multi_acc_without_sign_iter = multi_account_create_db.find(multi_account_withsign_iter->multi_account_pre_trx_id);
					auto op = current_trx.operations[0].get<eth_multi_account_create_record_operation>();
					bool cold_state = false;
					bool hot_state = false;
					if (op.multi_pubkey_type == "cold"){
						cold_state = true;
					}
					else {
						hot_state = true;
					}
					modify(*multi_account_withsign_iter, [&](eth_multi_account_trx_object& obj) {
						obj.cold_trx_success = cold_state | obj.cold_trx_success;
						obj.hot_trx_success = hot_state | obj.hot_trx_success;
					});
					modify(*multi_acc_without_sign_iter, [&](eth_multi_account_trx_object& obj) {
						obj.cold_trx_success = cold_state | obj.cold_trx_success;
						obj.hot_trx_success = hot_state | obj.hot_trx_success;
					});
					create<eth_multi_account_trx_object>([&](eth_multi_account_trx_object& obj) {
						obj.multi_account_create_trx_id = current_trx_id;
						obj.multi_account_pre_trx_id = pre_trx_id;
						obj.object_transaction = current_trx;
						obj.op_type = op_type;
						obj.state = sol_multi_account_ethchain_create;
					});
				}
				else if (op_type == operation::tag<miner_generate_multi_asset_operation>::value) {
					auto& multi_account_create_db = get_index_type<eth_multi_account_trx_index>().indices().get<by_mulaccount_trx_id>();
					auto& multi_account_create_range = get_index_type<eth_multi_account_trx_index>().indices().get<by_pre_trx_id>().equal_range(pre_trx_id);
					auto with_sign_trx_iter = multi_account_create_db.find(pre_trx_id);
					auto without_sign_iter = multi_account_create_db.find(with_sign_trx_iter->multi_account_pre_trx_id);
					modify(*with_sign_trx_iter, [&](eth_multi_account_trx_object& obj) {
						obj.state = sol_create_comlete;
					});
					modify(*without_sign_iter, [&](eth_multi_account_trx_object& obj) {
						obj.state = sol_create_comlete;
					});
					for (auto item : boost::make_iterator_range(multi_account_create_range.first, multi_account_create_range.second)) {
						auto iter = multi_account_create_db.find(item.multi_account_create_trx_id);
						modify(*iter, [&](eth_multi_account_trx_object& obj) {
							obj.state = sol_create_comlete;
						});
					}
					create<eth_multi_account_trx_object>([&](eth_multi_account_trx_object& obj) {
						obj.multi_account_create_trx_id = current_trx_id;
						obj.multi_account_pre_trx_id = pre_trx_id;
						obj.object_transaction = current_trx;
						obj.op_type = op_type;
						obj.state = sol_create_comlete;
					});
				}
			}FC_CAPTURE_AND_RETHROW((pre_trx_id)(current_trx_id))
		}
	}
}
