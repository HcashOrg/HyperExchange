#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>

namespace graphene {
	namespace chain {
		struct coldhot_transfer_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			string multi_account_withdraw;
			string multi_account_deposit;
			string amount;
			string asset_symbol;
			asset fee;
			asset_id_type asset_id;
			string memo;
			address guard;
			guard_member_id_type guard_id;
			address fee_payer()const {
				return guard;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		struct coldhot_transfer_without_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			
			transaction_id_type coldhot_trx_id;
			fc::variant_object coldhot_trx_original_chain;
			int withdraw_account_count;
			miner_id_type miner_broadcast;
			address miner_address;
			asset fee;
			string asset_symbol;
			asset_id_type asset_id;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		struct coldhot_transfer_with_sign_operation :public base_operation {

			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			
			transaction_id_type coldhot_trx_id;
			fc::variant_object coldhot_trx_original_chain;
			//TODO:refund balance in the situation that channel account tie to formal account
			guard_member_id_type sign_guard;
			string asset_symbol;
			asset fee;
			address guard_address;
			string coldhot_transfer_sign;

			address fee_payer()const {
				return guard_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
			}
		};
		struct coldhot_transfer_combine_sign_operation :public base_operation {

			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			
			fc::variant_object coldhot_trx_original_chain;
			vector<transaction_id_type> signed_trx_ids;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
			transaction_id_type coldhot_transfer_trx_id;
			address miner_address;
			string asset_symbol;
			asset fee;
			string original_trx_id;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		struct coldhot_transfer_result_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			
			crosschain::hd_trx coldhot_trx_original_chain;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
			asset fee;
			address miner_address;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		struct coldhot_cancel_transafer_transaction_operation : public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};

			transaction_id_type trx_id;
			//TODO:refund balance in the situation that channel account tie to formal account
			asset fee;
			address guard;
			guard_member_id_type guard_id;
			address fee_payer()const {
				return guard;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		struct coldhot_cancel_uncombined_trx_operaion :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};

			transaction_id_type trx_id;
			asset fee;
			address guard;
			guard_member_id_type guard_id;
			address fee_payer()const {
				return guard;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const { return share_type(0); };
		};
	}
}
FC_REFLECT(graphene::chain::coldhot_transfer_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_transfer_operation, (fee)(multi_account_withdraw)(multi_account_deposit)(amount)(asset_symbol)(asset_id)(memo)(guard)(guard_id))
FC_REFLECT(graphene::chain::coldhot_transfer_without_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_transfer_without_sign_operation, (fee)(coldhot_trx_id)(withdraw_account_count)(coldhot_trx_original_chain)(miner_broadcast)(miner_address)(asset_symbol)(asset_id))
FC_REFLECT(graphene::chain::coldhot_transfer_with_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_transfer_with_sign_operation, (fee)(coldhot_trx_id)(coldhot_trx_original_chain)(sign_guard)(asset_symbol)(guard_address)(coldhot_transfer_sign))
FC_REFLECT(graphene::chain::coldhot_transfer_combine_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_transfer_combine_sign_operation, (fee)(coldhot_trx_original_chain)(signed_trx_ids)(original_trx_id)(miner_broadcast)(coldhot_transfer_trx_id)(miner_address)(asset_symbol))
FC_REFLECT(graphene::chain::coldhot_transfer_result_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_transfer_result_operation, (fee)(coldhot_trx_original_chain)(miner_broadcast)(miner_address))
FC_REFLECT(graphene::chain::coldhot_cancel_transafer_transaction_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::coldhot_cancel_transafer_transaction_operation, (fee)(trx_id)(guard)(guard_id))
FC_REFLECT(graphene::chain::coldhot_cancel_uncombined_trx_operaion::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::coldhot_cancel_uncombined_trx_operaion, (fee)(trx_id)(guard)(guard_id))
