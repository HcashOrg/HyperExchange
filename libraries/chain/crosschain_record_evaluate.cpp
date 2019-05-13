#include <graphene/chain/crosschain_record_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <fc/smart_ref_impl.hpp>

namespace graphene {
	namespace chain {
		void_result crosschain_record_evaluate::do_evaluate(const crosschain_record_operation& o) {
			try {
				auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(o.cross_chain_trx.trx_id);
				if (deposit_to_link_trx != deposit_db.end()) {
					if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
						FC_ASSERT(false, "deposit transaction exist");
					}
				}

				auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
				auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
				FC_ASSERT(tunnel_itr != tunnel_idx.end());
				FC_ASSERT(tunnel_itr->owner == o.deposit_address);
				auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				FC_ASSERT(asset_itr != asset_idx.end());
				FC_ASSERT(asset_itr->symbol == o.cross_chain_trx.asset_symbol);
				FC_ASSERT(asset_itr->allow_withdraw_deposit == true);
				std::string to_account;
				if (("ETH" == o.cross_chain_trx.asset_symbol) || (o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos)) {
					auto pos = o.cross_chain_trx.to_account.find("|");
					if (pos != o.cross_chain_trx.to_account.npos) {
						to_account = o.cross_chain_trx.to_account.substr(0, pos);
					}
					else {
						to_account = o.cross_chain_trx.to_account;
					}
				}
				else {
					to_account = o.cross_chain_trx.to_account;
				}
				auto multisig_obj = db().get_multisgi_account(to_account, o.cross_chain_trx.asset_symbol);

				//auto multisig_obj = db().get_multisgi_account(o.cross_chain_trx.to_account,o.cross_chain_trx.asset_symbol);
				FC_ASSERT(multisig_obj.valid(), "multisig address does not exist.");
				FC_ASSERT(multisig_obj->effective_block_num > 0, "multisig address has not worked yet.");
				auto current_obj = db().get_current_multisig_account(o.cross_chain_trx.asset_symbol);
				FC_ASSERT(current_obj.valid(), "there is no worked multisig address.");

				if (multisig_obj->effective_block_num != current_obj->effective_block_num)
				{
					FC_ASSERT(db().head_block_num() - multisig_obj->end_block <= 20000, "this multi addr has expired");

				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.cross_chain_trx.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				auto obj = db().get_asset(o.cross_chain_trx.asset_symbol);
				FC_ASSERT(obj.valid());
				auto descrip = obj->options.description;
				bool bCheckTransactionValid = false;
				if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
					return void_result();
				if (o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos) {
					auto temp_hdtx = o.cross_chain_trx;
					temp_hdtx.asset_symbol = temp_hdtx.asset_symbol + '|' + descrip;
					bCheckTransactionValid = hdl->validate_link_trx(temp_hdtx);
				}
				else {
					if (db().head_block_num() >= CROSSCHAIN_RECORD_EVALUATE_220000)
					{
						bCheckTransactionValid = hdl->validate_link_trx_v1(o.cross_chain_trx);
					}
					else {
						bCheckTransactionValid = hdl->validate_link_trx(o.cross_chain_trx);
					}
					
				}
				FC_ASSERT(bCheckTransactionValid, "This transaction doesnt valid");
				
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
			
		}
		void_result crosschain_record_evaluate::do_apply(const crosschain_record_operation& o) {
			try {
				auto& tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_binded_account>();
				//			for (auto tunnel_itr : boost::make_iterator_range(tunnel_idx_range.first,tunnel_idx_range.second)){
				auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
				auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				asset amount;
				amount = asset_itr->amount_from_string(o.cross_chain_trx.amount);
				/*
				if ((o.cross_chain_trx.asset_symbol == "ETH") || (o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos)) {
					auto pr = asset_itr->precision;
					auto temp_amount = o.cross_chain_trx.amount;
					std::string handle_amount;
					auto float_pos = temp_amount.find('.');
					auto float_amount = temp_amount.substr(float_pos + 1);
					std::string float_temp;
					if (float_amount.size() >= 1)
					{
						int breakCondition = 0;
						for (int i = pr; i >= 1; i--)
						{
							float_temp += float_amount[breakCondition];
							++breakCondition;
							if (breakCondition >= float_amount.size())
							{
								break;
							}
						}
					}
					amount = asset_itr->amount_from_string(temp_amount.substr(0, float_pos) + '.' + float_temp.substr(0, asset_itr->precision));
				}
				else {
					amount = asset_itr->amount_from_string(o.cross_chain_trx.amount);
				}*/
				db().adjust_balance(tunnel_itr->owner, amount);
				db().adjust_deposit_to_link_trx(o.cross_chain_trx);
				//			}
				db().modify(asset_itr->dynamic_asset_data_id(db()), [amount](asset_dynamic_data_object& d) {
					d.current_supply += amount.amount;
				});
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
			
		}
		void crosschain_record_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_evaluate::do_evaluate(const crosschain_withdraw_operation& o) {
			try {
				database& d = db();
				if (trx_state->_trx == nullptr)
					return void_result();

				auto trx_db = d.get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>().equal_range(trx_state->_trx->id());
				int i = 0;
				for (auto trx_iter : boost::make_iterator_range(trx_db.first, trx_db.second)) {
					i++;
				}
				FC_ASSERT((i == 0), "This Transaction exist");
				//FC_ASSERT(tunnel_itr->bind_account != o.crosschain_account);
				auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				FC_ASSERT(asset_itr != asset_idx.end());
				FC_ASSERT(asset_itr->allow_withdraw_deposit, "this asset does not allow to withdraw and deposit.");
				auto float_pos = o.amount.find(".");
				if (float_pos != o.amount.npos)
				{
					FC_ASSERT(o.amount.substr(float_pos + 1).size() <= asset_itr->precision, "amount precision error");
				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				bool valid_address = hdl->validate_address(o.crosschain_account);
				FC_ASSERT(valid_address, "crosschain address isn`t valid");
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				//FC_ASSERT(asset_itr->symbol == o.asset_symbol);
				const auto& dyn_ac = asset_itr->dynamic_data(db());
				if (db().head_block_num() > CROSSCHAIN_RECORD_EVALUATE_480000)
				{
					if (asset_itr->symbol.find("ERC") != asset_itr->symbol.npos)
					{
						FC_ASSERT(asset_itr->amount_from_string(o.amount).amount > dyn_ac.withdraw_limition);
					}
					else {
						FC_ASSERT(asset_itr->amount_from_string(o.amount).amount >= dyn_ac.withdraw_limition);
					}
				}
				else {
					FC_ASSERT(asset_itr->amount_from_string(o.amount).amount >= dyn_ac.withdraw_limition);
				}
				
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}
		void_result crosschain_withdraw_evaluate::do_apply(const crosschain_withdraw_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				database& d = db();
				auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
				auto asset_itr = asset_idx.find(o.asset_id);
				auto amount = asset_itr->amount_from_string(o.amount);
				d.adjust_balance(o.withdraw_account, -amount);
				d.adjust_crosschain_transaction(transaction_id_type(), trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_operation>::value), withdraw_without_sign_trx_uncreate);

				d.modify(asset_itr->dynamic_asset_data_id(d), [amount](asset_dynamic_data_object& d) {
					d.current_supply -= amount.amount;
				});
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}

		void crosschain_withdraw_evaluate::pay_fee() {
			return void();
		}

		void_result crosschain_withdraw_result_evaluate::do_evaluate(const crosschain_withdraw_result_operation& o) {
			try {
				auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
				auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
				string crosschain_trx_id = o.cross_chain_trx.trx_id;
				if (o.cross_chain_trx.asset_symbol == "ETH" || o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos)
				{
					if (o.cross_chain_trx.trx_id.find('|') != o.cross_chain_trx.trx_id.npos)
					{
						auto pos = o.cross_chain_trx.trx_id.find('|');
						crosschain_trx_id = o.cross_chain_trx.trx_id.substr(pos + 1);
					}
				}
				auto combine_trx_iter = originaldb.find(boost::make_tuple(crosschain_trx_id, combine_op_number));
				FC_ASSERT(combine_trx_iter != originaldb.end());
				auto & tx_db_objs = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto tx_without_sign_iter = tx_db_objs.find(combine_trx_iter->relate_transaction_id);
				FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "user cross chain tx exist error");
				auto current_blockNum = db().get_dynamic_global_properties().head_block_number;
				if(current_blockNum > CROSSCHAIN_RECORD_EVALUATE_1000000)
				{
					if (o.cross_chain_trx.asset_symbol == "ETH" || o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos)
					{
						FC_ASSERT(combine_trx_iter->trx_state == withdraw_eth_guard_sign);
					}
				}
				crosschain_fee = tx_without_sign_iter->crosschain_fee;
				for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
					auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
					FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
				}
				auto & deposit_db = db().get_index_type<acquired_crosschain_index>().indices().get<by_acquired_trx_id>();
				auto deposit_to_link_trx = deposit_db.find(crosschain_trx_id);
				if (deposit_to_link_trx != deposit_db.end()) {
					FC_ASSERT((deposit_to_link_trx->acquired_transaction_state == acquired_trx_uncreate), "deposit transaction exist");
					// 				if (deposit_to_link_trx->acquired_transaction_state != acquired_trx_uncreate) {
					// 					FC_ASSERT("deposit transaction exist");
					// 				}
				}
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.cross_chain_trx.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				auto obj = db().get_asset(o.cross_chain_trx.asset_symbol);
				FC_ASSERT(obj.valid());
				if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
					return void_result();
				auto descrip = obj->options.description;
				bool bCheckTransactionValid = false;
				if (o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos) {
					auto temp_hdtx = o.cross_chain_trx;
					temp_hdtx.asset_symbol = temp_hdtx.asset_symbol + '|' + descrip;
					bCheckTransactionValid = hdl->validate_link_trx(temp_hdtx);
				}
				else {
					bCheckTransactionValid = hdl->validate_link_trx(o.cross_chain_trx);
				}
				FC_ASSERT(bCheckTransactionValid, "This transaction doesnt valid");
				//hdl->validate_link_trx(o.cross_chain_trx);
				//auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
				//auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.to_account, o.cross_chain_trx.asset_symbol));
	// 			auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
	// 			auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
	// 			auto combine_trx_iter = originaldb.find(boost::make_tuple(o.cross_chain_trx.trx_id, combine_op_number));
	// 
	// 			FC_ASSERT(combine_trx_iter != originaldb.end());
				//FC_ASSERT(tunnel_itr != tunnel_idx.end());
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
			

		}
		void_result crosschain_withdraw_result_evaluate::do_apply(const crosschain_withdraw_result_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				auto& originaldb = db().get_index_type<crosschain_trx_index>().indices().get<by_original_id_optype>();
				auto combine_op_number = uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value);
				string crosschain_trx_id = o.cross_chain_trx.trx_id;
				if (o.cross_chain_trx.asset_symbol == "ETH" || o.cross_chain_trx.asset_symbol.find("ERC") != o.cross_chain_trx.asset_symbol.npos)
				{
					if (o.cross_chain_trx.trx_id.find('|') != o.cross_chain_trx.trx_id.npos)
					{
						auto pos = o.cross_chain_trx.trx_id.find('|');
						crosschain_trx_id = o.cross_chain_trx.trx_id.substr(pos + 1);
					}
				}
				auto combine_trx_iter = originaldb.find(boost::make_tuple(crosschain_trx_id, combine_op_number));
				db().adjust_crosschain_confirm_trx(o.cross_chain_trx);
				db().adjust_crosschain_transaction(combine_trx_iter->relate_transaction_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_result_operation>::value), withdraw_transaction_confirm);
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}

		void crosschain_withdraw_result_evaluate::pay_fee() {
			auto& d = db();
			d.modify_current_collected_fee(crosschain_fee);
		}
		bool crosschain_withdraw_result_evaluate::if_evluate()
		{
			return true;
		}
		int split(const std::string& str, std::vector<std::string>& ret_, std::string sep)
		{
			if (str.empty())
			{
				return 0;
			}

			std::string tmp;
			std::string::size_type pos_begin = str.find_first_not_of(sep);
			std::string::size_type comma_pos = 0;

			while (pos_begin != std::string::npos)
			{
				comma_pos = str.find(sep, pos_begin);
				if (comma_pos != std::string::npos)
				{
					tmp = str.substr(pos_begin, comma_pos - pos_begin);
					pos_begin = comma_pos + sep.length();
				}
				else
				{
					tmp = str.substr(pos_begin);
					pos_begin = comma_pos;
				}

				if (!tmp.empty())
				{
					ret_.push_back(tmp);
					tmp.clear();
				}
			}
			return 0;
		}
		void_result crosschain_withdraw_without_sign_evaluate::do_evaluate(const crosschain_withdraw_without_sign_operation& o) {
			try {
				auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto obj = db().get_asset(o.asset_symbol);
				FC_ASSERT(obj.valid());
				for (const auto& crosschain_withdraw_trx_id : o.ccw_trx_ids) {
					auto crosschain_withdraw_trx = trx_db.find(crosschain_withdraw_trx_id);
					FC_ASSERT(crosschain_withdraw_trx != trx_db.end(), "Source Transaction doesn`t exist");
					if (db().head_block_num() >= CROSSCHAIN_RECORD_EVALUATE_310000)
					{
						FC_ASSERT(crosschain_withdraw_trx->trx_state == withdraw_without_sign_trx_uncreate,"this transaction has been created.");
					}
					
				}
				if (o.asset_symbol == "BCH") {
					auto temp_trx = o.withdraw_source_trx;
					FC_ASSERT(temp_trx.contains("hex"));
					FC_ASSERT(temp_trx.contains("trx"));
					auto tx = temp_trx["trx"].get_object();
					auto vins = tx["vin"].get_array();
					for (int index = 0; index < vins.size(); index++) {
						FC_ASSERT(vins[index].get_object().contains("amount"), "No amount in bch vin");
					}
				}
				if (trx_state->_trx == nullptr)
					return void_result();
				auto trx_current_iter = trx_db.find(trx_state->_trx->id());
				FC_ASSERT(trx_current_iter == trx_db.end(), "Crosschain transaction has been created");
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
					return void_result();
				crosschain_trx create_trxs;
				if ((o.asset_symbol.find("ERC") != o.asset_symbol.npos))
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
					mul_obj.set("turn_without_eth_sign", o.withdraw_source_trx);
					create_trxs = hdl->turn_trxs(fc::variant_object(mul_obj));
				}
				else if (o.asset_symbol == "ETH") {
					create_trxs = hdl->turn_trxs(fc::variant_object("turn_without_eth_sign", o.withdraw_source_trx));
				}
				else if (o.asset_symbol == "USDT") {
					FC_ASSERT(o.ccw_trx_ids.size() == 1,"USDT cant combine");
					create_trxs = hdl->turn_trxs(fc::variant_object("turn_without_usdt_sign", o.withdraw_source_trx));
					FC_ASSERT(create_trxs.trxs.size() == 1,"USDT transaction count error");
				
					auto to_accounts = create_trxs.trxs.begin()->second.from_account;
					std::vector<std::string> sep_vec;
					split(to_accounts, sep_vec, "|");
					FC_ASSERT((sep_vec.size() > 1 && sep_vec.size() <= 3),"Not USDT trx");
					FC_ASSERT(sep_vec[0] != "-", "No from account get");
					int toAccountCheck = 0;
					std::string fee_change_address = "";
					for (int i = 1;i  < sep_vec.size();++i)
					{
						auto sep_pos = sep_vec[i].find(',');
						FC_ASSERT(sep_pos != sep_vec[i].npos,"USDT turn error");
						auto address = sep_vec[i].substr(0, sep_pos);
						auto amount = fc::to_double(sep_vec[i].substr(sep_pos + 1));
						FC_ASSERT(amount > 0, "USDT Amount error");
						if (amount <= 0.00000546){
							toAccountCheck += 1;
						}
						else {
							fee_change_address = address;
						}
					}
					FC_ASSERT(toAccountCheck == 1, "usdt doesnt get to_account");
					if (fee_change_address != "")
					{
						auto multisig_obj = db().get_multisgi_account(fee_change_address, o.asset_symbol);
						FC_ASSERT(multisig_obj.valid(), "multisig address does not exist.");
					}
				}
				else {
					create_trxs = hdl->turn_trxs(o.withdraw_source_trx);
				}


				auto trx_itr_relate = trx_db.find(trx_state->_trx->id());
				FC_ASSERT(trx_itr_relate == trx_db.end(), "Crosschain transaction has been created");
				auto asset_fee = db().get_asset(o.asset_symbol)->dynamic_data(db()).fee_pool;

				std::map<string, share_type> fees_cal;
				std::map<std::string, asset> all_balances;
				const auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_symbol>();
				for (auto& one_trx_id : o.ccw_trx_ids)
				{
					auto trx_itr = trx_db.find(one_trx_id);
					FC_ASSERT(trx_itr != trx_db.end());
					FC_ASSERT(trx_itr->real_transaction.operations.size() == 1);
					const auto withdraw_op = trx_itr->real_transaction.operations[0].get<crosschain_withdraw_evaluate::operation_type>();

					FC_ASSERT(create_trxs.trxs.count(withdraw_op.crosschain_account) == 1);
					auto one_trx = create_trxs.trxs[withdraw_op.crosschain_account];

					FC_ASSERT(one_trx.to_account == withdraw_op.crosschain_account);
					{

						const auto asset_itr = asset_idx.find(withdraw_op.asset_symbol);
						FC_ASSERT(asset_itr != asset_idx.end());
						FC_ASSERT(one_trx.asset_symbol == withdraw_op.asset_symbol);
						if (all_balances.count(withdraw_op.crosschain_account) > 0)
						{
							all_balances[withdraw_op.crosschain_account] = all_balances[withdraw_op.crosschain_account] + asset_itr->amount_from_string(withdraw_op.amount);
						}
						else
						{
							all_balances[withdraw_op.crosschain_account] = asset_itr->amount_from_string(withdraw_op.amount);
						}
						fees_cal[withdraw_op.crosschain_account] += asset_fee;
						//FC_ASSERT(asset_itr->amount_from_string(one_trx.amount).amount == asset_itr->amount_from_string(withdraw_op.amount).amount);
					}
				}
				FC_ASSERT(all_balances.size() == create_trxs.trxs.size());
				const auto opt_asset = db().get(o.asset_id);
				char temp_fee[1024];
				string format = "%." + fc::variant(opt_asset.precision).as_string() + "f";
				std::sprintf(temp_fee, format.c_str(), create_trxs.fee);
				auto cross_fee = opt_asset.amount_from_string(graphene::utilities::remove_zero_for_str_amount(temp_fee));
				share_type total_amount = 0;
				share_type total_op_amount = 0;
				for (auto& one_balance : all_balances)
				{
					FC_ASSERT(create_trxs.trxs.count(one_balance.first) == 1);
					FC_ASSERT(one_balance.second.amount == obj->amount_from_string(create_trxs.trxs.at(one_balance.first).amount).amount + fees_cal.at(one_balance.first));
					total_amount += one_balance.second.amount;
				}

				for (auto& trx : create_trxs.trxs)
				{
					total_op_amount += opt_asset.amount_from_string(trx.second.amount).amount;
				}
				FC_ASSERT(o.crosschain_fee.amount >= 0);
				FC_ASSERT(total_amount == (total_op_amount + o.crosschain_fee.amount + cross_fee.amount));
				//FC_ASSERT(o.ccw_trx_ids.size() == create_trxs.size());


				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
			
		}
		void_result crosschain_withdraw_without_sign_evaluate::do_apply(const crosschain_withdraw_without_sign_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				for (const auto& one_trx_id : o.ccw_trx_ids)
				{
					db().adjust_crosschain_transaction(one_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_without_sign_operation>::value), withdraw_without_sign_trx_create, o.ccw_trx_ids);
				}
				auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto iter = trx_db.find(trx_state->_trx->id());
				db().modify(*iter, [&o](crosschain_trx_object& a) {
					a.crosschain_fee = o.crosschain_fee;
				});
				db().modify(db().get(asset_id_type(o.asset_id)).dynamic_asset_data_id(db()), [&o](asset_dynamic_data_object& d) {
					d.current_supply += o.crosschain_fee.amount;
				});

				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}
		void crosschain_withdraw_without_sign_evaluate::pay_fee() {
		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_evaluate(const crosschain_withdraw_combine_sign_operation& o) {
			try {
				auto & tx_db_objs = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto trx_iter = tx_db_objs.find(*(o.signed_trx_ids.begin()));
				FC_ASSERT(trx_iter != tx_db_objs.end());
				auto tx_without_sign_iter = tx_db_objs.find(trx_iter->relate_transaction_id);
				if (trx_state->_trx == nullptr)
					return void_result();
				auto tx_combine_sign_iter = tx_db_objs.find(trx_state->_trx->id());
				FC_ASSERT(tx_without_sign_iter != tx_db_objs.end(), "without sign tx exist error");
				FC_ASSERT(tx_combine_sign_iter == tx_db_objs.end(), "combine sign tx has create");
				for (auto tx_user_transaciton_id : tx_without_sign_iter->all_related_origin_transaction_ids) {
					auto tx_user_crosschain_iter = tx_db_objs.find(tx_user_transaciton_id);
					FC_ASSERT(tx_user_crosschain_iter != tx_db_objs.end(), "user cross chain tx exist error");
					if (db().head_block_num() > CROSSCHAIN_RECORD_EVALUATE_480000)
					{
						FC_ASSERT(tx_user_crosschain_iter->trx_state == withdraw_without_sign_trx_create);
					}
				}
				if (o.asset_symbol == "ETH" || o.asset_symbol.find("ERC") != o.asset_symbol.npos){
					if (db().head_block_num() > CROSSCHAIN_RECORD_EVALUATE_680000){ 
						FC_ASSERT(o.cross_chain_trx.contains("without_sign"));
						FC_ASSERT(o.cross_chain_trx.contains("signer"));
					}
				}
				FC_ASSERT(trx_state->_trx->operations.size() == 1, "operation error");
				FC_ASSERT(o.get_fee().valid(), "doenst set fee");
				fee = *o.get_fee();
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_apply(const crosschain_withdraw_combine_sign_operation& o) {
			try {
				auto &trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto trx_iter = trx_db.find(*(o.signed_trx_ids.begin()));
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_crosschain_transaction(trx_iter->relate_transaction_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_combine_sign_operation>::value), withdraw_combine_trx_create);

				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
					return void_result();
				if (o.asset_symbol == "ETH" || o.asset_symbol.find("ERC") != o.asset_symbol.npos)
				{

				}
				else {
					fc::async([hdl, o] {
						hdl->broadcast_transaction(o.cross_chain_trx); });
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}
		void crosschain_withdraw_combine_sign_evaluate::pay_fee() {
		}
		bool crosschain_withdraw_combine_sign_evaluate::if_evluate()
		{
			return true;
		}
		void_result crosschain_withdraw_with_sign_evaluate::do_evaluate(const crosschain_withdraw_with_sign_operation& o) {
			//auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			//auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			try {
				auto& trx_db = db().get_index_type<crosschain_trx_index>().indices().get<by_transaction_id>();
				auto crosschain_without_sign_trx_iter = trx_db.find(o.ccw_trx_id);
				FC_ASSERT(crosschain_without_sign_trx_iter != trx_db.end(), "without sign trx doesn`t exist");
				if (trx_state->_trx == nullptr)
					return void_result();
				auto trx_itr = trx_db.find(trx_state->_trx->id());
				FC_ASSERT(trx_itr == trx_db.end(), "This sign tx exist");
				vector<crosschain_trx_object> all_signs;
				set<std::string> signs;
				auto sign_range = db().get_index_type<crosschain_trx_index >().indices().get<by_relate_trx_id>().equal_range(o.ccw_trx_id);
				for (auto sign_obj : boost::make_iterator_range(sign_range.first, sign_range.second)) {
					FC_ASSERT(sign_obj.trx_state <= withdraw_sign_trx && sign_obj.trx_state >= withdraw_without_sign_trx_create);
					if (sign_obj.trx_state != withdraw_sign_trx) {
						continue;
					}
					std::string temp_sign;
					auto op = sign_obj.real_transaction.operations[0];
					auto sign_op = op.get<crosschain_withdraw_with_sign_operation>();
					//std::string sig = sign_op.ccw_trx_signature;
					std::string sig = fc::variant(sign_op.sign_guard).as_string();
					signs.insert(sig);
				}
				// 			db().get_index_type<crosschain_trx_index >().inspect_all_objects([&](const object& obj)
				// 			{
				// 				const crosschain_trx_object& p = static_cast<const crosschain_trx_object&>(obj);
				// 				if (p.relate_transaction_id == o.ccw_trx_id&&p.trx_state == withdraw_sign_trx) {
				// 					all_signs.push_back(p);
				// 				}
				// 			});
				// 			//set<std::string> signs;
				// 			for (const auto & signing : all_signs) {
				// 				std::string temp_sign;
				// 				auto op = signing.real_transaction.operations[0];
				// 				auto sign_op = op.get<crosschain_withdraw_with_sign_operation>();
				//                                 std::string sig = sign_op.ccw_trx_signature;
				// 				signs.insert(sig);
				// 			}
							//auto sign_iter = signs.find(o.ccw_trx_signature);
				auto sign_iter = signs.find(fc::variant(o.sign_guard).as_string());
				FC_ASSERT(sign_iter == signs.end(), "Guard has sign this transaction");
				//db().get_index_type<crosschain_trx_index>().indices().get<by_trx_relate_type_stata>();
				//auto trx_iter = trx_db.find(boost::make_tuple(o.ccw_trx_id, withdraw_sign_trx));
				auto& manager = graphene::crosschain::crosschain_manager::get_instance();
				if (!manager.contain_crosschain_handles(o.asset_symbol))
					return void_result();
				auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
				if (!hdl->valid_config())
					return void_result();
				if (fc::time_point::now() > db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
					return void_result();
				crosschain_trx hd_trxs;
				std::string from_account;
				if (o.asset_symbol != "ETH" && o.asset_symbol.find("ERC") == o.asset_symbol.npos)
				{
					from_account = hdl->get_from_address(o.withdraw_source_trx);
				}
				else {
					hd_trxs = hdl->turn_trxs(fc::variant_object("turn_without_eth_sign", o.withdraw_source_trx));
					FC_ASSERT(hd_trxs.trxs.size() >= 1);
					auto crosschain_tx = hd_trxs.trxs.begin()->second;
					from_account = crosschain_tx.from_account;
				}
				vector<multisig_address_object>  senator_pubks = db().get_multi_account_senator(from_account, o.asset_symbol);
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
				if (o.asset_symbol == "BCH")
				{
					auto without_trx = crosschain_without_sign_trx_iter->real_transaction;
					FC_ASSERT(without_trx.operations.size() == 1);
					auto without_sign_op = without_trx.operations[0].get<crosschain_withdraw_without_sign_operation>();
					auto source_trx = fc::json::to_string(without_sign_op.withdraw_source_trx);
					FC_ASSERT(hdl->validate_transaction(senator_pubks[index].new_pubkey_hot, 
						multisig_account_obj.redeemScript_hot, 
						o.ccw_trx_signature + "|" + source_trx)
						|| hdl->validate_transaction(senator_pubks[index].new_pubkey_cold, 
							multisig_account_obj.redeemScript_cold,
							o.ccw_trx_signature + "|" + source_trx));
				}
				else {
					FC_ASSERT(hdl->validate_transaction(senator_pubks[index].new_pubkey_hot, multisig_account_obj.redeemScript_hot, o.ccw_trx_signature) || hdl->validate_transaction(senator_pubks[index].new_pubkey_cold, multisig_account_obj.redeemScript_cold, o.ccw_trx_signature));
				}
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
			
		}
		void_result crosschain_withdraw_with_sign_evaluate::do_apply(const crosschain_withdraw_with_sign_operation& o) {
			try {
				FC_ASSERT(trx_state->_trx != nullptr);
				db().adjust_crosschain_transaction(o.ccw_trx_id, trx_state->_trx->id(), *(trx_state->_trx), uint64_t(operation::tag<crosschain_withdraw_with_sign_operation>::value), withdraw_sign_trx);
				return void_result();
			}FC_CAPTURE_AND_RETHROW((o));
		}
		void crosschain_withdraw_with_sign_evaluate::pay_fee() {

		}
	}
}
