#include <graphene/chain/crosschain_record_evaluate.hpp>
#include <graphene/chain/database.hpp>


namespace graphene {
	namespace chain {
		void_result crosschain_record_evaluate::do_evaluate(const crosschain_record_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			hdl->validate_link_trx(o.cross_chain_trx);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.from_account, o.cross_chain_trx.asset_symbol));
			FC_ASSERT(tunnel_itr == tunnel_idx.end());
			return void_result();
		}
		void_result crosschain_record_evaluate::do_apply(const crosschain_record_operation& o) {

		}

		void crosschain_record_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_evaluate::do_evaluate(const crosschain_withdraw_operation& o) {
			FC_ASSERT(o.amount > 0);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.withdraw_account, o.asset_symbol));
			FC_ASSERT(tunnel_itr == tunnel_idx.end());
			FC_ASSERT(tunnel_itr->)
		}
		void_result crosschain_withdraw_evaluate::do_apply(const crosschain_withdraw_operation& o) {

		}

		void crosschain_withdraw_evaluate::pay_fee() {

		}
		void_result crosschain_withdraw_result_evaluate::do_evaluate(const crosschain_withdraw_result_operation& o) {
			auto& manager = graphene::crosschain::crosschain_manager::get_instance();
			auto hdl = manager.get_crosschain_handle(std::string(o.cross_chain_trx.asset_symbol));
			hdl->validate_link_trx(o.cross_chain_trx);
			auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
			auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.cross_chain_trx.to_account, o.cross_chain_trx.asset_symbol));
			FC_ASSERT(tunnel_itr == tunnel_idx.end());
			return void_result();
		}
		void_result crosschain_withdraw_result_evaluate::do_apply(const crosschain_withdraw_result_operation& o) {

		}

		void crosschain_withdraw_result_evaluate::pay_fee() {

		}
	}
}
