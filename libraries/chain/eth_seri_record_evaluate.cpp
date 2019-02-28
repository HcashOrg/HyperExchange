#include <graphene/chain/eth_seri_record_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/transaction_object.hpp>
namespace graphene {
	namespace chain {

		void_result eth_series_multi_sol_create_evaluator::do_evaluate(const eth_series_multi_sol_create_operation& o) {
			try {
				//validate miner
				const auto& miners = db().get_index_type<miner_index>().indices().get<by_id>();
				auto miner = miners.find(o.miner_broadcast);
				FC_ASSERT(miner != miners.end());
				const auto& accounts = db().get_index_type<account_index>().indices().get<by_id>();
				const auto acct = accounts.find(miner->miner_account);
				FC_ASSERT(acct->addr == o.miner_broadcast_addrss);

				const auto& assets = db().get_index_type<asset_index>().indices().get<by_symbol>();
				FC_ASSERT(assets.find(o.chain_type) != assets.end());
				if (trx_state->_trx == nullptr)
					return void_result();

				FC_ASSERT(trx_state->_trx->operations.size() == 1);
				//if the multi-addr correct or not.
				vector<string> symbol_addrs_cold;
				vector<string> symbol_addrs_hot;
				vector<pair<account_id_type, pair<string, string>>> eth_guard_account_ids;
				const auto& addr = db().get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
				auto addr_range = addr.equal_range(boost::make_tuple(o.chain_type));
				std::for_each(
					addr_range.first, addr_range.second, [&symbol_addrs_cold, &symbol_addrs_hot,&eth_guard_account_ids](const multisig_address_object& obj) {
					if (obj.multisig_account_pair_object_id == multisig_account_pair_id_type())
					{
						symbol_addrs_cold.push_back(obj.new_pubkey_cold);
						symbol_addrs_hot.push_back(obj.new_pubkey_hot);
						auto account_pair = make_pair(obj.guard_account, make_pair(obj.new_pubkey_hot, obj.new_pubkey_cold));
						eth_guard_account_ids.push_back(account_pair);
					}
				}
				);
				const auto hot_range = db().get_index_type<eth_multi_account_trx_index>().indices().get<by_eth_hot_multi>().equal_range(o.multi_hot_address);
				const auto cold_range = db().get_index_type<eth_multi_account_trx_index>().indices().get<by_eth_cold_multi>().equal_range(o.multi_cold_address);
				FC_ASSERT(hot_range.first == hot_range.second);
				FC_ASSERT(cold_range.first == cold_range.second);
				const auto& guard_dbs = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto sign_guard_iter = guard_dbs.find(o.guard_to_sign);
				FC_ASSERT(sign_guard_iter != guard_dbs.end());
				FC_ASSERT(sign_guard_iter->senator_type == PERMANENT);
				FC_ASSERT(sign_guard_iter->formal == true);
				auto& instance = graphene::crosschain::crosschain_manager::get_instance();
				if (!instance.contain_crosschain_handles(o.chain_type))
					return void_result();
				auto crosschain_interface = instance.get_crosschain_handle(o.chain_type);
				if (!crosschain_interface->valid_config())
					return void_result();
				auto ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(o.chain_type);
				std::string temp_address_cold;
				std::string temp_address_hot;
				for (auto public_cold : symbol_addrs_cold)
				{
					temp_address_cold += ptr->get_address_by_pubkey(public_cold);
				}
				for (auto public_hot : symbol_addrs_hot)
				{
					temp_address_hot += ptr->get_address_by_pubkey(public_hot);
				}
				FC_ASSERT(o.multi_hot_address == temp_address_hot);
				FC_ASSERT(o.multi_cold_address == temp_address_cold);
				
				std::string temp_cold, temp_hot;
				for (auto guard_account_id : eth_guard_account_ids) {
					if (guard_account_id.first == sign_guard_iter->guard_member_account){
						temp_hot = ptr->get_address_by_pubkey(guard_account_id.second.first);
						temp_cold = ptr->get_address_by_pubkey(guard_account_id.second.second);
						break;
					}
				}
				FC_ASSERT(temp_hot != "", "guard donst has hot address");
				FC_ASSERT(temp_cold != "", "guard donst has cold address");
				FC_ASSERT(o.guard_sign_hot_address == temp_hot);
				FC_ASSERT(o.guard_sign_cold_address == temp_cold);

				const auto& guard_ids = db().get_global_properties().active_committee_members;
				FC_ASSERT(symbol_addrs_cold.size() == guard_ids.size() && symbol_addrs_hot.size() == guard_ids.size()&& eth_guard_account_ids.size() == guard_ids.size());
				std::map<std::string, std::string> multi_addr_cold;
				std::map<std::string, std::string> multi_addr_hot;
				if (db().head_block_num() >= ETH_SERI_RECORD_EVALUATE_480000){
					std::string hot_nonce_temp = o.hot_nonce;
					std::string gas_price = "5000000000";
					auto sep_pos = o.hot_nonce.find('|');
					if (sep_pos != o.hot_nonce.npos) {
						gas_price = o.hot_nonce.substr(sep_pos + 1);
						hot_nonce_temp = o.hot_nonce.substr(0, sep_pos);
					}
					multi_addr_cold = crosschain_interface->create_multi_sig_account(temp_cold + '|' + o.cold_nonce+'|'+ gas_price, symbol_addrs_cold, (symbol_addrs_cold.size() * 2 / 3 + 1));
					multi_addr_hot = crosschain_interface->create_multi_sig_account(temp_hot + '|' + hot_nonce_temp + '|' + gas_price, symbol_addrs_hot, (symbol_addrs_hot.size() * 2 / 3 + 1));
				}
				else {
					multi_addr_cold = crosschain_interface->create_multi_sig_account(temp_cold + '|' + o.cold_nonce, symbol_addrs_cold, (symbol_addrs_cold.size() * 2 / 3 + 1));
					multi_addr_hot = crosschain_interface->create_multi_sig_account(temp_hot + '|' + o.hot_nonce, symbol_addrs_hot, (symbol_addrs_hot.size() * 2 / 3 + 1));
				}
				
				FC_ASSERT(o.multi_account_tx_without_sign_cold != "","eth multi acocunt cold trx error");
				FC_ASSERT(o.multi_account_tx_without_sign_hot != "", "eth multi acocunt ,hot trx error");
				FC_ASSERT(o.multi_account_tx_without_sign_cold == multi_addr_cold[temp_cold]);
				FC_ASSERT(o.multi_account_tx_without_sign_hot == multi_addr_hot[temp_hot]);
			}FC_CAPTURE_AND_RETHROW((o))
		}
		object_id_type eth_series_multi_sol_create_evaluator::do_apply(const eth_series_multi_sol_create_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_eths_multi_account_record(transaction_id_type(), trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eth_series_multi_sol_create_operation>::value));
			return object_id_type();
		}
		void eth_series_multi_sol_create_evaluator::pay_fee() {

		}
		void_result eth_series_multi_sol_guard_sign_evaluator::do_evaluate(const eths_multi_sol_guard_sign_operation& o) {
			try {
				const auto& guard_db = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_to_sign);
				FC_ASSERT(guard_iter != guard_db.end(), "This Guard doesnt exist");
				const auto& accounts = db().get_index_type<account_index>().indices().get<by_id>();
				const auto acct = accounts.find(guard_iter->guard_member_account);
				FC_ASSERT(acct->addr == o.guard_sign_address);
				const auto& multi_account_create_db = db().get_index_type<eth_multi_account_trx_index>().indices().get<by_mulaccount_trx_id>();
				auto multi_account_create_iter = multi_account_create_db.find(o.sol_without_sign_txid);
				FC_ASSERT(multi_account_create_iter != multi_account_create_db.end());
				FC_ASSERT(multi_account_create_iter->state == sol_create_need_guard_sign);
				if (trx_state->_trx == nullptr)
					return void_result();
				FC_ASSERT(trx_state->_trx->operations.size() == 1);
				FC_ASSERT(multi_account_create_iter->object_transaction.operations.size() == 1);
				FC_ASSERT(multi_account_create_iter->symbol == o.chain_type);
				const auto create_multi_account_op = multi_account_create_iter->object_transaction.operations[0].get<eth_series_multi_sol_create_operation>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.chain_type)){
					return void_result();
				}
				auto handle = manager.get_crosschain_handle(o.chain_type);
				if (!handle->valid_config()){
					return void_result();
				}
				
				auto hot_trx = handle->turn_trx(fc::variant_object("get_without_sign", o.multi_hot_sol_guard_sign));
				auto cold_trx = handle->turn_trx(fc::variant_object("get_without_sign", o.multi_cold_sol_guard_sign));
				auto hot_trx_id = (handle->turn_trx(fc::variant_object("get_with_sign", o.multi_hot_sol_guard_sign))).trx_id;
				auto cold_trx_id = (handle->turn_trx(fc::variant_object("get_with_sign", o.multi_cold_sol_guard_sign))).trx_id;
				FC_ASSERT(hot_trx_id == o.multi_hot_trxid);
				FC_ASSERT(cold_trx_id == o.multi_cold_trxid);
				FC_ASSERT(create_multi_account_op.multi_account_tx_without_sign_cold == cold_trx.to_account);
				FC_ASSERT(create_multi_account_op.multi_account_tx_without_sign_hot == hot_trx.to_account);
				
			}FC_CAPTURE_AND_RETHROW((o))
		}
		object_id_type eth_series_multi_sol_guard_sign_evaluator::do_apply(const eths_multi_sol_guard_sign_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_eths_multi_account_record(o.sol_without_sign_txid, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_multi_sol_guard_sign_operation>::value));
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return object_id_type();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return object_id_type();
			}
		    handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.multi_hot_sol_guard_sign));
			handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.multi_cold_sol_guard_sign)); 		
			return object_id_type();
		}
		void eth_series_multi_sol_guard_sign_evaluator::pay_fee() {

		}
		void_result eths_guard_change_signer_evaluator::do_evaluate(const eths_guard_change_signer_operation& o) {
			try {
				const auto& guard_db = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_to_sign);
				FC_ASSERT(guard_iter != guard_db.end(), "This Guard doesnt exist");
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				const auto& crosschain_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto trx_iter = crosschain_db.find(o.combine_trx_id);
				FC_ASSERT(trx_iter != crosschain_db.end());
				FC_ASSERT(trx_iter->trx_state == withdraw_eth_guard_need_sign);
				auto combine_op = trx_iter->real_transaction.operations[0].get<crosschain_withdraw_combine_sign_operation>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.chain_type)) {
					return void_result();
				}
				auto handle = manager.get_crosschain_handle(o.chain_type);
				if (!handle->valid_config()) {
					return void_result();
				}
				auto change_signer_input = handle->turn_trx(fc::variant_object("change_signer", o.signed_crosschain_trx));
				auto combine_trx_input = handle->turn_trx(fc::variant_object("change_signer", combine_op.cross_chain_trx["without_sign"].as_string()));
				FC_ASSERT(combine_trx_input.to_account == change_signer_input.to_account);
				
			}FC_CAPTURE_AND_RETHROW((o))

		}
		object_id_type eths_guard_change_signer_evaluator::do_apply(const eths_guard_change_signer_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_crosschain_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_guard_change_signer_operation>::value), withdraw_eth_guard_sign);
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return object_id_type();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return object_id_type();
			}
			if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
			{
				return object_id_type();
			}
			fc::async([handle,o] {
				handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.signed_crosschain_trx));
				 });
			return object_id_type();
		}
		void_result eths_guard_sign_final_evaluator::do_evaluate(const eths_guard_sign_final_operation& o) {
			try{
				const auto& guard_db = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_to_sign);
				FC_ASSERT(guard_iter != guard_db.end(), "This Guard doesnt exist");
				const auto& crosschain_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				const auto trx_iter = crosschain_db.find(o.combine_trx_id);
				FC_ASSERT(trx_iter != crosschain_db.end());
				auto combine_op = trx_iter->real_transaction.operations[0].get<crosschain_withdraw_combine_sign_operation>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.chain_type)) {
					return void_result();
				}
				auto handle = manager.get_crosschain_handle(o.chain_type);
				if (!handle->valid_config()) {
					return void_result();
				}
				auto combine_trx = handle->turn_trx(fc::variant_object("get_without_sign", o.signed_crosschain_trx));
				FC_ASSERT(combine_trx.to_account == combine_op.cross_chain_trx["without_sign"].as_string());
				
			}FC_CAPTURE_AND_RETHROW((o))
			
		}
		object_id_type eths_guard_sign_final_evaluator::do_apply(const eths_guard_sign_final_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_crosschain_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_guard_sign_final_operation>::value), withdraw_eth_guard_sign);
			/*
			const auto& trx_history_db = db().get_index_type<trx_index>().indices().get<by_trx_id>();
			const auto trx_history_iter = trx_history_db.find(o.combine_trx_id);
			auto current_blockNum = db().get_dynamic_global_properties().head_block_number;
			if (current_blockNum < 436)
			{
				if (trx_history_iter->block_num + 100 < current_blockNum) {
					std::cout << "sign final not put in" << std::endl;
				}
				else {
					db().adjust_crosschain_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_guard_sign_final_operation>::value), withdraw_eth_guard_sign);
					std::cout << "sign final has put in" << std::endl;
				}
			}
			else {
				if (trx_history_iter->block_num + 100 >= current_blockNum) {
					std::cout << "sign final not put in" << std::endl;
				}
				else {
					db().adjust_crosschain_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_guard_sign_final_operation>::value), withdraw_eth_guard_sign);
					std::cout << "sign final has put in" << std::endl;
				}
			}
			*/
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return object_id_type();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return object_id_type();
			}
			if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
			{
				return object_id_type();
			}
			fc::async([handle,o] {
				handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.signed_crosschain_trx)); });
			//db().adjust_eths_multi_account_record();
			return object_id_type();
		}
		void_result eths_guard_coldhot_change_signer_evaluator::do_evaluate(const eths_guard_coldhot_change_signer_operation& o) {
			try {
				const auto& guard_db = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_to_sign);
				FC_ASSERT(guard_iter != guard_db.end(), "This Guard doesnt exist");
				FC_ASSERT(guard_iter->senator_type == PERMANENT);
				const auto& coldhot_db = db().get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				const auto trx_iter = coldhot_db.find(o.combine_trx_id);
				FC_ASSERT(trx_iter != coldhot_db.end());
				auto combine_op = trx_iter->current_trx.operations[0].get<coldhot_transfer_combine_sign_operation>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.chain_type)) {
					return void_result();
				}
				auto handle = manager.get_crosschain_handle(o.chain_type);
				if (!handle->valid_config()) {
					return void_result();
				}
				auto change_signer_input = handle->turn_trx(fc::variant_object("change_signer", o.signed_crosschain_trx));
				auto combine_trx_input = handle->turn_trx(fc::variant_object("change_signer", combine_op.coldhot_trx_original_chain["without_sign"].as_string()));
				FC_ASSERT(combine_trx_input.to_account == change_signer_input.to_account);
				/*auto coldhot_trx = handle->turn_trx(fc::variant_object("get_without_sign", o.signed_crosschain_trx));
				FC_ASSERT(coldhot_trx.to_account == combine_op.coldhot_trx_original_chain["without_sign"].as_string());*/

			}FC_CAPTURE_AND_RETHROW((o))

		}
		object_id_type eths_guard_coldhot_change_signer_evaluator::do_apply(const eths_guard_coldhot_change_signer_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_coldhot_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_guard_coldhot_change_signer_operation>::value));
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return object_id_type();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return object_id_type();
			}
			if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
			{
				return object_id_type();
			}
			fc::async([handle,o] {handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.signed_crosschain_trx)); });
			//db().adjust_eths_multi_account_record();
			return object_id_type();
		}
		void eths_guard_coldhot_change_signer_evaluator::pay_fee() {

		}
		void_result eths_coldhot_guard_sign_final_evaluator::do_evaluate(const eths_coldhot_guard_sign_final_operation& o) {
			try {
				const auto& guard_db = db().get_index_type<guard_member_index>().indices().get<by_id>();
				auto guard_iter = guard_db.find(o.guard_to_sign);
				FC_ASSERT(guard_iter != guard_db.end(), "This Guard doesnt exist");
				const auto& coldhot_db = db().get_index_type<coldhot_transfer_index>().indices().get<by_current_trx_id>();
				const auto trx_iter = coldhot_db.find(o.combine_trx_id);
				FC_ASSERT(trx_iter != coldhot_db.end());
				auto combine_op = trx_iter->current_trx.operations[0].get<coldhot_transfer_combine_sign_operation>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.chain_type)) {
					return void_result();
				}
				auto handle = manager.get_crosschain_handle(o.chain_type);
				if (!handle->valid_config()) {
					return void_result();
				}
				auto coldhot_trx = handle->turn_trx(fc::variant_object("get_without_sign", o.signed_crosschain_trx));
				FC_ASSERT(coldhot_trx.to_account == combine_op.coldhot_trx_original_chain["without_sign"].as_string());
				
			}FC_CAPTURE_AND_RETHROW((o))

		}
		object_id_type eths_coldhot_guard_sign_final_evaluator::do_apply(const eths_coldhot_guard_sign_final_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_coldhot_transaction(o.combine_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eths_coldhot_guard_sign_final_operation>::value));
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return object_id_type();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return object_id_type();
			}
			if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
			{
				return object_id_type();
			}
			fc::async([handle,o] {handle->broadcast_transaction(fc::variant_object("trx", "0x" + o.signed_crosschain_trx)); });
			//db().adjust_eths_multi_account_record();
			return object_id_type();
		}
		void eths_coldhot_guard_sign_final_evaluator::pay_fee() {

		}
		void eths_guard_sign_final_evaluator::pay_fee() {

		}
		void eths_guard_change_signer_evaluator::pay_fee() {

		}
		void_result eth_multi_account_create_record_evaluator::do_evaluate(const eth_multi_account_create_record_operation& o) {
			const auto& miners = db().get_index_type<miner_index>().indices().get<by_id>();
			auto miner = miners.find(o.miner_broadcast);
			FC_ASSERT(miner != miners.end());
			const auto& accounts = db().get_index_type<account_index>().indices().get<by_id>();
			const auto acct = accounts.find(miner->miner_account);
			FC_ASSERT(acct->addr == o.miner_address);
			const auto& multi_account_create_db = db().get_index_type<eth_multi_account_trx_index>().indices().get<by_mulaccount_trx_id>();
			auto multi_account_create_sign_iter = multi_account_create_db.find(o.pre_trx_id);
			FC_ASSERT(multi_account_create_sign_iter != multi_account_create_db.end());
			FC_ASSERT(multi_account_create_sign_iter->state == sol_create_guard_signed);
			FC_ASSERT(multi_account_create_sign_iter->object_transaction.operations.size() == 1);
			auto multi_acc_without_sign_iter = multi_account_create_db.find(multi_account_create_sign_iter->multi_account_pre_trx_id);
			FC_ASSERT(multi_acc_without_sign_iter != multi_account_create_db.end());
			FC_ASSERT(multi_acc_without_sign_iter->state == sol_create_guard_signed);
			FC_ASSERT(multi_acc_without_sign_iter->object_transaction.operations.size() == 1);
			FC_ASSERT(multi_account_create_sign_iter->symbol == o.chain_type);

			const auto create_multi_account_op = multi_account_create_sign_iter->object_transaction.operations[0].get<eths_multi_sol_guard_sign_operation>();
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			if (!manager.contain_crosschain_handles(o.chain_type)) {
				return void_result();
			}
			auto handle = manager.get_crosschain_handle(o.chain_type);
			if (!handle->valid_config()) {
				return void_result();
			}
			if (o.multi_pubkey_type == "cold")
			{
				auto cold_hdtx = handle->turn_trx(fc::variant_object("get_with_sign", create_multi_account_op.multi_cold_sol_guard_sign));
				FC_ASSERT(multi_account_create_sign_iter->cold_trx_success == false);
				FC_ASSERT(cold_hdtx.trx_id == o.eth_multi_account_trx.trx_id);
			}
			else {
				auto hot_hdtx = handle->turn_trx(fc::variant_object("get_with_sign", create_multi_account_op.multi_hot_sol_guard_sign));
				FC_ASSERT(multi_account_create_sign_iter->hot_trx_success == false);
				FC_ASSERT(hot_hdtx.trx_id == o.eth_multi_account_trx.trx_id);
			}

			
		}
		object_id_type eth_multi_account_create_record_evaluator::do_apply(const eth_multi_account_create_record_operation& o) {
			FC_ASSERT(trx_state->_trx != nullptr);
			db().adjust_eths_multi_account_record(o.pre_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<eth_multi_account_create_record_operation>::value));
			return object_id_type();
		}
		void eth_multi_account_create_record_evaluator::pay_fee() {

		}
		void_result senator_change_acquire_trx_evaluator::do_evaluate(const senator_change_acquire_trx_operation& o) {
			const database& d = db();

			const auto& guard_db = d.get_index_type<guard_member_index>().indices().get<by_id>();
			auto guard_iter = guard_db.find(o.guard_id);
			FC_ASSERT(guard_iter->senator_type == PERMANENT);
			FC_ASSERT(guard_iter != guard_db.end(), "cant find this guard");
			const auto& account_db = d.get_index_type<account_index>().indices().get<by_id>();
			auto account_iter = account_db.find(guard_iter->guard_member_account);
			FC_ASSERT(account_iter != account_db.end(), "cant find this account");
			FC_ASSERT(account_iter->addr == o.guard_address, "guard address error");
			const auto& trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			const auto sign_final_iter = trx_db.find(o.change_transaction_id);
			FC_ASSERT(sign_final_iter != trx_db.end(), "transaction not exist.");
			FC_ASSERT(sign_final_iter->trx_state == withdraw_eth_guard_sign, "cross chain trx state error");
			FC_ASSERT(sign_final_iter->real_transaction.operations.size() == 1, "operation size error");
			const auto& trx_history_db = d.get_index_type<trx_index>().indices().get<by_trx_id>();
			const auto trx_history_iter = trx_history_db.find(o.change_transaction_id);
			FC_ASSERT(trx_history_iter != trx_history_db.end());
			auto current_blockNum = d.get_dynamic_global_properties().head_block_number;
			FC_ASSERT(trx_history_iter->block_num + 20 < current_blockNum);

			auto op = sign_final_iter->real_transaction.operations[0];
			FC_ASSERT(op.which() == operation::tag<eths_guard_sign_final_operation>::value, "operation type error");
			auto sign_final_op = op.get<eths_guard_sign_final_operation>();
			string crosschain_trx_id = sign_final_op.signed_crosschain_trx_id;
			if (sign_final_op.chain_type == "ETH" || sign_final_op.chain_type.find("ERC") != sign_final_op.chain_type.npos)
			{
				if (crosschain_trx_id.find('|') != crosschain_trx_id.npos)
				{
					auto pos = crosschain_trx_id.find('|');
					crosschain_trx_id = crosschain_trx_id.substr(pos + 1);
				}
			}
			auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
			auto deposit_to_link_trx = deposit_db.find(crosschain_trx_id);
			FC_ASSERT(deposit_to_link_trx != deposit_db.end());
			auto multisig_obj = db().get_multisgi_account(deposit_to_link_trx->handle_trx.from_account, sign_final_op.chain_type);
			FC_ASSERT(multisig_obj.valid(), "multisig address does not exist.");
			FC_ASSERT(multisig_obj->effective_block_num > 0, "multisig address has not worked yet.");
		}
		object_id_type senator_change_acquire_trx_evaluator::do_apply(const senator_change_acquire_trx_operation& o) {
			const auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
			const auto sign_final_iter = trx_db.find(o.change_transaction_id);
			auto sign_final_op = sign_final_iter->real_transaction.operations[0].get<eths_guard_sign_final_operation>();
			string crosschain_trx_id = sign_final_op.signed_crosschain_trx_id;
			if (sign_final_op.chain_type == "ETH" || sign_final_op.chain_type.find("ERC") != sign_final_op.chain_type.npos)
			{
				if (crosschain_trx_id.find('|') != crosschain_trx_id.npos)
				{
					auto pos = crosschain_trx_id.find('|');
					crosschain_trx_id = crosschain_trx_id.substr(pos + 1);
				}
			}
			auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
			auto deposit_to_link_trx = deposit_db.find(crosschain_trx_id);
			db().modify(*deposit_to_link_trx, [&](acquired_crosschain_trx_object& obj) {
				obj.acquired_transaction_state = acquired_trx_uncreate;
			});
		}
		void senator_change_acquire_trx_evaluator::pay_fee() {

		}
	}
}