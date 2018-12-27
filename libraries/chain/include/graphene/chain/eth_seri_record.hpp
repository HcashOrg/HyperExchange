#pragma once
#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>
namespace graphene {
	namespace chain {
		struct eth_series_multi_sol_create_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			miner_id_type miner_broadcast;
			address miner_broadcast_addrss;
			guard_member_id_type guard_to_sign;
			std::string multi_cold_address;
			std::string multi_hot_address;
			std::string multi_account_tx_without_sign_hot;
			std::string multi_account_tx_without_sign_cold;
			std::string cold_nonce;
			std::string hot_nonce;
			std::string chain_type;
			std::string guard_sign_hot_address;
			std::string guard_sign_cold_address;
			address fee_payer()const {
				return miner_broadcast_addrss;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_broadcast_addrss, 1));
			}
		};
		struct eths_multi_sol_guard_sign_operation : public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			transaction_id_type sol_without_sign_txid;
			guard_member_id_type guard_to_sign;
			address guard_sign_address;
			std::string multi_cold_trxid;
			std::string multi_hot_trxid;
			std::string multi_hot_sol_guard_sign;
			std::string multi_cold_sol_guard_sign;
			std::string chain_type;
			address fee_payer()const {
				return guard_sign_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_sign_address, 1));
			}
		};
		//gather record
		struct eth_multi_account_create_record_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			miner_id_type miner_broadcast;
			address miner_address;
			std::string chain_type;
			std::string multi_pubkey_type;
			graphene::crosschain::hd_trx eth_multi_account_trx;
			transaction_id_type pre_trx_id;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		
		struct eth_seri_guard_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			fc::variant_object eth_guard_sign_trx;
			address guard_address;
			guard_member_id_type guard_to_sign;
			address fee_payer()const {
				return guard_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
			}
		};
		struct eths_guard_sign_final_operation:public base_operation{
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			guard_member_id_type guard_to_sign;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address guard_address;
			std::string chain_type;
			address fee_payer()const {
				return guard_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
			}
		};
		struct eths_guard_change_signer_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			guard_member_id_type guard_to_sign;
			std::string new_signer;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address guard_address;
			std::string chain_type;
			address fee_payer()const {
				return guard_address;
			}
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		
		struct eths_guard_coldhot_change_signer_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			guard_member_id_type guard_to_sign;
			std::string new_signer;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address guard_address;
			std::string chain_type;
			address fee_payer()const {
				return guard_address;
			}
			share_type      calculate_fee(const fee_parameters_type& k)const;
		};
		struct eths_cancel_unsigned_transaction_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			address    guard_address;
			guard_member_id_type guard_id;
			transaction_id_type cancel_trx_id;
			address fee_payer()const {
				return guard_address;
			}
			share_type  calculate_fee(const fee_parameters_type& k)const { return 0; }
		};
		struct eths_coldhot_guard_sign_final_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 0.001 * GRAPHENE_HXCHAIN_PRECISION;
			};
			asset fee;
			guard_member_id_type guard_to_sign;
			transaction_id_type combine_trx_id;
			std::string signed_crosschain_trx;
			std::string signed_crosschain_trx_id;
			address guard_address;
			std::string chain_type;
			address fee_payer()const {
				return guard_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
			}
		};
		
	}
}
FC_REFLECT(graphene::chain::eth_seri_guard_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eth_seri_guard_sign_operation, (fee)(eth_guard_sign_trx)(guard_address)(guard_to_sign))
FC_REFLECT(graphene::chain::eth_series_multi_sol_create_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eth_series_multi_sol_create_operation, (fee)(guard_to_sign)(miner_broadcast)(miner_broadcast_addrss)(multi_cold_address)
																	(multi_hot_address)(multi_account_tx_without_sign_hot)(multi_account_tx_without_sign_cold)(cold_nonce)(hot_nonce)(chain_type)(guard_sign_hot_address)(guard_sign_cold_address))
FC_REFLECT(graphene::chain::eth_multi_account_create_record_operation::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::eth_multi_account_create_record_operation, (fee)(miner_broadcast)(miner_address)(chain_type)(multi_pubkey_type)(eth_multi_account_trx)(eth_multi_account_trx)(pre_trx_id))
FC_REFLECT(graphene::chain::eths_multi_sol_guard_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_multi_sol_guard_sign_operation, (fee)(sol_without_sign_txid)(guard_to_sign)(guard_sign_address)
																(multi_hot_sol_guard_sign)(multi_cold_trxid)(multi_hot_trxid)(multi_cold_sol_guard_sign)(chain_type))
FC_REFLECT(graphene::chain::eths_guard_sign_final_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_guard_sign_final_operation, (fee)(guard_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(chain_type)(signed_crosschain_trx)(guard_address))
FC_REFLECT(graphene::chain::eths_coldhot_guard_sign_final_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_coldhot_guard_sign_final_operation, (fee)(guard_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(chain_type)(signed_crosschain_trx)(guard_address))
FC_REFLECT(graphene::chain::eths_guard_change_signer_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_guard_change_signer_operation, (fee)(guard_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(new_signer)(chain_type)(signed_crosschain_trx)(guard_address))
FC_REFLECT(graphene::chain::eths_guard_coldhot_change_signer_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_guard_coldhot_change_signer_operation, (fee)(guard_to_sign)(signed_crosschain_trx_id)(combine_trx_id)(new_signer)(chain_type)(signed_crosschain_trx)(guard_address))

FC_REFLECT(graphene::chain::eths_cancel_unsigned_transaction_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::eths_cancel_unsigned_transaction_operation, (fee)(guard_address)(guard_id)(cancel_trx_id))