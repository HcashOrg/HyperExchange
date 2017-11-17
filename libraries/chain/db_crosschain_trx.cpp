#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_crosschain_transaction(transaction_id_type relate_transaction_id,
		transaction_id_type transaction_id,
		uint64_t op_type,
		transaction_stata trx_state) {
			if (op_type == operation::tag<crosschain_withdraw_evaluate::operation_type>::value){
				auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto trx_itr = trx_db.find(transaction_id);
				FC_ASSERT(trx_itr == trx_db.end(), "This Transaction exist");
				create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
					obj.op_type = op_type;
					obj.relate_transaction_id = transaction_id_type();
					obj.transaction_id = transaction_id;
					obj.trx_state = withdraw_without_sign_trx_uncreate;
				});
			}
			else if (op_type == operation::tag<crosschain_withdraw_without_sign_evaluate::operation_type>::value){
				auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto trx_itr = trx_db.find(relate_transaction_id);
				FC_ASSERT(trx_itr != trx_db.end(), "Source Transaction doesn`t exist");
				auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>();
				auto trx_itr_relate = trx_db_relate.find(transaction_id);
				FC_ASSERT(trx_itr_relate == trx_db_relate.end(), "Crosschain transaction has been created");
				create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
					obj.op_type = op_type;
					obj.relate_transaction_id = relate_transaction_id;
					obj.transaction_id = transaction_id;
					obj.trx_state = withdraw_without_sign_trx_create;
				});
				modify<crosschain_trx_object>(*trx_itr, [](crosschain_trx_object& obj) {
					obj.trx_state = withdraw_without_sign_trx_create;
				});
			}
			else if (op_type == operation::tag<crosschain_withdraw_combine_sign_evaluate::operation_type>::value) {

			}
			else if (op_type == operation::tag<crosschain_withdraw_with_sign_evaluate::operation_type>::value) {
				auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>();
				auto trx_itr_relate = trx_db_relate.find(relate_transaction_id);
				FC_ASSERT(trx_itr_relate != trx_db_relate.end(), "Source trx doesnt exist");
				
				auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto trx_itr = trx_db.find(transaction_id);
				FC_ASSERT(trx_itr == trx_db.end(), "This sign tex is exist");
				
				create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
					obj.op_type = op_type;
					obj.relate_transaction_id = relate_transaction_id;
					obj.transaction_id = transaction_id;
					obj.trx_state = withdraw_sign_trx;
				});
			}
			else if (op_type == operation::tag<crosschain_withdraw_result_evaluate::operation_type>::value) {

			}
		}
		void database::create_result_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			vector<crosschain_trx_object> create_withdraw_trx;
			get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& o)
			{
				const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(o);
				if (p.trx_state == withdraw_without_sign_trx_uncreate){
					create_withdraw_trx.push_back(p);
				}
			});
			for (auto & cross_chain_trx : create_withdraw_trx){
				auto& trx_db = get_index_type<transaction_index>().indices().get<by_trx_id>();
				auto trx_itr = trx_db.find(cross_chain_trx.transaction_id);
				if (trx_itr == trx_db.end() || trx_itr->trx.operations.size() != 1){
					continue;
				}
				auto op = trx_itr->trx.operations[0];
				auto withop = op.get<crosschain_withdraw_evaluate::operation_type>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				auto hdl = manager.get_crosschain_handle(std::string(withop.asset_symbol));
				crosschain_withdraw_without_sign_operation trx_op;
				trx_op.withdraw_source_trx = hdl->create_multisig_transaction(std::string(withop.withdraw_account), withop.crosschain_account, withop.amount.value, withop.asset_symbol, withop.memo, false);
				trx_op.ccw_trx_id = trx_itr->trx.id();
				trx_op.miner_broadcast = miner;
				trx_op.asset_symbol = withop.asset_symbol;
				trx_op.asset_id = withop.asset_id;
				optional<miner_object> miner_iter = get(miner);
				optional<account_object> account_iter = get(miner_iter->miner_account);
				trx_op.miner_address = account_iter->addr;
				signed_transaction tx;
				
				uint32_t expiration_time_offset = 0;
				auto dyn_props = get_dynamic_global_properties();
				get_global_properties().parameters.current_fees->set_fee(operation(trx_op));
				tx.set_reference_block(dyn_props.head_block_id);
				tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
				tx.operations.push_back(trx_op);
				tx.validate();
				tx.sign(pk, get_chain_id());
				push_transaction(tx);
			}
		}
		void database::combine_sign_transaction(miner_id_type miner, fc::ecc::private_key pk) {
			vector<crosschain_trx_object> uncombine_trxs;
			map<transaction_id_type, vector<transaction_id_type>> uncombine_trxs_counts;
			get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& o)
			{
				const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(o);
				if (p.trx_state == withdraw_sign_trx) {
					uncombine_trxs.push_back(p);
				}
			});
			for (auto & trx: uncombine_trxs) {
				uncombine_trxs_counts[trx.relate_transaction_id].push_back(trx.transaction_id);
			}
			for (auto & trxs : uncombine_trxs_counts){
				// Use macro instead this magic number
				if (trxs.second.size() < 7){
					continue;
				}
				vector<string> combine_signature;
				bool transaction_err = false;
				for (auto & sign_trx:trxs.second){
					auto& trx_db = get_index_type<transaction_index>().indices().get<by_trx_id>();
					auto trx_itr = trx_db.find(sign_trx);
					if (trx_itr == trx_db.end() || trx_itr->trx.operations.size() != 1) {
						transaction_err = true;
						break;
					}
					auto op = trx_itr->trx.operations[0];
					auto with_sign_op = op.get<crosschain_withdraw_with_sign_evaluate::operation_type>();
					combine_signature.push_back(with_sign_op.ccw_trx_signature);
				}
				if (transaction_err){
					continue;
				}
				auto& trx_db = get_index_type<transaction_index>().indices().get<by_trx_id>();
				auto trx_itr = trx_db.find(trxs.first);
				if (trx_itr == trx_db.end() || trx_itr->trx.operations.size() != 1) {
					continue;
				}
				auto op = trx_itr->trx.operations[0];
				auto with_sign_op = op.get<crosschain_withdraw_without_sign_evaluate::operation_type>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				auto hdl = manager.get_crosschain_handle(std::string(with_sign_op.asset_symbol));
				crosschain_withdraw_combine_sign_operation trx_op;
				trx_op.cross_chain_trx = hdl->merge_multisig_transaction(with_sign_op.withdraw_source_trx, combine_signature);
				trx_op.signed_trx_ids.swap(trxs.second);
				trx_op.miner_broadcast = miner;
				optional<miner_object> miner_iter = get(miner);
				optional<account_object> account_iter = get(miner_iter->miner_account);
				trx_op.miner_address = account_iter->addr;
				trx_op.withdraw_trx = with_sign_op.ccw_trx_id;
				signed_transaction tx;
				uint32_t expiration_time_offset = 0;
				auto dyn_props = get_dynamic_global_properties();
				get_global_properties().parameters.current_fees->set_fee(operation(trx_op));
				tx.set_reference_block(dyn_props.head_block_id);
				tx.set_expiration(dyn_props.time + fc::seconds(30 + expiration_time_offset));
				tx.operations.push_back(trx_op);
				tx.validate();
				tx.sign(pk, get_chain_id());
				push_transaction(tx);
			}
		}
	}
}