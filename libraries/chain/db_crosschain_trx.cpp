#include <graphene/chain/database.hpp>
#include <graphene/chain/crosschain_trx_object.hpp>
namespace graphene {
	namespace chain {
		void database::adjust_crosschain_transaction(transaction_id_type relate_transaction_id,
		transaction_id_type transaction_id,
		signed_transaction real_transaction,
		uint64_t op_type,
		transaction_stata trx_state
		) {

			try{
				if (op_type == operation::tag<crosschain_withdraw_evaluate::operation_type>::value) {
					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_itr = trx_db.find(transaction_id);
					FC_ASSERT(trx_itr == trx_db.end(), "This Transaction exist");
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = transaction_id_type();
						obj.transaction_id = transaction_id;
						obj.real_transaction = real_transaction;
						obj.trx_state = withdraw_without_sign_trx_uncreate;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_without_sign_evaluate::operation_type>::value) {
					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_itr = trx_db.find(relate_transaction_id);
					FC_ASSERT(trx_itr != trx_db.end(), "Source Transaction doesn`t exist");
					auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>();
					auto trx_itr_relate = trx_db_relate.find(transaction_id);
					FC_ASSERT(trx_itr_relate == trx_db_relate.end(), "Crosschain transaction has been created");
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.trx_state = withdraw_without_sign_trx_create;
					});
					auto& trx_db_new = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_iter_new = trx_db_new.find(relate_transaction_id);
					modify(*trx_iter_new, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_without_sign_trx_create;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_combine_sign_evaluate::operation_type>::value) {
					auto& trx_db_relate = get_index_type<crosschain_trx_index>().indices().get<by_relate_trx_id>();
					auto trx_itr_relate = trx_db_relate.find(relate_transaction_id);
					FC_ASSERT(trx_itr_relate != trx_db_relate.end(), "Source trx doesnt exist");

					auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_itr = trx_db.find(transaction_id);
					FC_ASSERT(trx_itr == trx_db.end(), "This sign tex is exist");
					create<crosschain_trx_object>([&](crosschain_trx_object& obj) {
						obj.op_type = op_type;
						obj.relate_transaction_id = relate_transaction_id;
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.trx_state = withdraw_combine_trx_create;
					});
					auto& trx_db_new = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
					auto trx_iter_new = trx_db_new.find(relate_transaction_id);
					modify(*trx_iter_new, [&](crosschain_trx_object& obj) {
						obj.trx_state = withdraw_combine_trx_create;
					});
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
						obj.real_transaction = real_transaction;
						obj.transaction_id = transaction_id;
						obj.trx_state = withdraw_sign_trx;
					});
				}
				else if (op_type == operation::tag<crosschain_withdraw_result_evaluate::operation_type>::value) {

				}
			}FC_CAPTURE_AND_RETHROW((relate_transaction_id)(transaction_id))
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
				if (cross_chain_trx.real_transaction.id() != cross_chain_trx.transaction_id || cross_chain_trx.real_transaction.operations.size() < 1){
					continue;
				}
				auto op = cross_chain_trx.real_transaction.operations[0];
				auto withop = op.get<crosschain_withdraw_evaluate::operation_type>();
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				auto hdl = manager.get_crosschain_handle(std::string(withop.asset_symbol));
				crosschain_withdraw_without_sign_operation trx_op;
				trx_op.withdraw_source_trx = hdl->create_multisig_transaction(std::string(withop.withdraw_account), withop.crosschain_account, withop.amount.value, withop.asset_symbol, withop.memo, false);
				trx_op.ccw_trx_id = cross_chain_trx.real_transaction.id();
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
			map<transaction_id_type, vector<transaction_id_type>> uncombine_trxs_counts;
			get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& o)
			{
				const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(o);
				if (p.trx_state == withdraw_sign_trx && p.relate_transaction_id != transaction_id_type()) {
					uncombine_trxs_counts[p.relate_transaction_id].push_back(p.transaction_id);
				}
			});
			for (auto & trxs : uncombine_trxs_counts){
				//TODO : Use macro instead this magic number
				if (trxs.second.size() < 7){
					continue;
				}
				set<string> combine_signature;
				auto& trx_db = get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				bool transaction_err = false;
				for (auto & sign_trx_id:trxs.second){
					auto trx_itr = trx_db.find(sign_trx_id);

					if (trx_itr == trx_db.end() || trx_itr->real_transaction.operations.size() < 1) {
						transaction_err = true;
						break;
					}
					auto op = trx_itr->real_transaction.operations[0];
					auto with_sign_op = op.get<crosschain_withdraw_with_sign_evaluate::operation_type>();
					combine_signature.insert(with_sign_op.ccw_trx_signature);
				}
				if (transaction_err){
					continue;
				}
				auto& trx_new_db = get_index_type<crosschain_trx_index>().indices().get<by_trx_relate_type_stata>();
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