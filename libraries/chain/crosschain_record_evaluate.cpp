#include <graphene/chain/crosschain_record_evaluate.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/transaction_object.hpp>
namespace graphene {
	namespace chain {
		void_result crosschain_record_evaluate::do_evaluate(const crosschain_record_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			hdl->validate_link_trx(o.cross_chain_trx);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
			FC_ASSERT(tunnel_itr != tunnel_idx.end());
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			FC_ASSERT(asset_itr != asset_idx.end());
			FC_ASSERT(asset_itr->symbol == o.cross_chain_trx.asset_symbol);
			return void_result();
		}
		void_result crosschain_record_evaluate::do_apply(const crosschain_record_operation& o) {
			database& d = db();
			auto &tunnel_idx = d.get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
			optional<account_object> account_iter = d.get(tunnel_itr->owner);
			d.adjust_balance(account_iter->addr, asset(o.cross_chain_trx.amount, o.asset_id));
			return void_result();
		}

		void crosschain_record_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_evaluate::do_evaluate(const crosschain_withdraw_operation& o) {
			FC_ASSERT(o.amount > 0);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_account_binding>();
			auto& acc = db().get_index_type<account_index>().indices().get<by_address>();
			auto id = acc.find(o.withdraw_account)->get_id();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(id, o.asset_symbol));
			FC_ASSERT(tunnel_itr != tunnel_idx.end());
			FC_ASSERT(tunnel_itr->bind_account == o.crosschain_account);
			auto & asset_idx = db().get_index_type<asset_index>().indices().get<by_id>();
			auto asset_itr = asset_idx.find(o.asset_id);
			FC_ASSERT(asset_itr != asset_idx.end());
			//FC_ASSERT(asset_itr->symbol == o.asset_symbol);
			return void_result();
		}
		void_result crosschain_withdraw_evaluate::do_apply(const crosschain_withdraw_operation& o) {
			database& d = db();
			d.adjust_balance(o.withdraw_account, asset(-o.amount, o.asset_id));
			d.adjust_crosschain_transaction(transaction_id_type(), trx_state->_trx->id(), uint64_t(operation::tag<crosschain_withdraw_operation>::value), withdraw_without_sign_trx_uncreate);
			return void_result();
		}

		void crosschain_withdraw_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_result_evaluate::do_evaluate(const crosschain_withdraw_result_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			hdl->validate_link_trx(o.cross_chain_trx);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.to_account, o.cross_chain_trx.asset_symbol));
			FC_ASSERT(tunnel_itr != tunnel_idx.end());
			return void_result();
		}
		void_result crosschain_withdraw_result_evaluate::do_apply(const crosschain_withdraw_result_operation& o) {
			return void_result();
		}

		void crosschain_withdraw_result_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_without_sign_evaluate::do_evaluate(const crosschain_withdraw_without_sign_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			auto create_trx = hdl->turn_trx(o.withdraw_source_trx);
			auto &trx_db = db().get_index_type<transaction_index>().indices().get<by_trx_id>();
			auto trx_itr = trx_db.find(o.ccw_trx_id);
			FC_ASSERT(trx_itr != trx_db.end());
			FC_ASSERT(trx_itr->trx.operations.size() == 1);
			for (const auto & op : trx_itr->trx.operations) {
				const auto withdraw_op = op.get<crosschain_withdraw_evaluate::operation_type>();
				FC_ASSERT(create_trx.to_account == withdraw_op.crosschain_account);
				FC_ASSERT(create_trx.amount == withdraw_op.amount);
				FC_ASSERT(create_trx.asset_symbol == withdraw_op.asset_symbol);
			}
			return void_result();
		}
		void_result crosschain_withdraw_without_sign_evaluate::do_apply(const crosschain_withdraw_without_sign_operation& o) {
			db().adjust_crosschain_transaction(trx_state->_trx->id(), o.ccw_trx_id, uint64_t(operation::tag<crosschain_withdraw_without_sign_operation>::value), withdraw_without_sign_trx_create);

			return void_result();
		}
		void crosschain_withdraw_combine_sign_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_evaluate(const crosschain_withdraw_combine_sign_operation& o) {
			return void_result();
		}
		void_result crosschain_withdraw_combine_sign_evaluate::do_apply(const crosschain_withdraw_combine_sign_operation& o) {
			return void_result();
		}
		void crosschain_withdraw_without_sign_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_with_sign_evaluate::do_evaluate(const crosschain_withdraw_with_sign_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.asset_symbol));
			return void_result();
		}
		void_result crosschain_withdraw_with_sign_evaluate::do_apply(const crosschain_withdraw_with_sign_operation& o) {
			db().adjust_crosschain_transaction(o.ccw_trx_id, trx_state->_trx->id(), uint64_t(operation::tag<crosschain_withdraw_with_sign_operation>::value), withdraw_sign_trx);
			return void_result();
		}
		void crosschain_withdraw_with_sign_evaluate::pay_fee() {

		}
	}
}
