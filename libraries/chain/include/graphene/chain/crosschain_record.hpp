#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/crosschain/crosschain_impl.hpp>
namespace graphene {
	namespace chain {
		struct crosschain_record_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			crosschain::hd_trx cross_chain_trx;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
			address miner_address;
			asset_id_type asset_id;
			string asset_symbol;
			asset fee;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		}; 
		struct crosschain_withdraw_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			address withdraw_account;
			string amount;
			string asset_symbol;
			asset fee;
			asset_id_type asset_id;
			string crosschain_account;
			string memo;
			address fee_payer()const {
				return withdraw_account;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, withdraw_account, 1));
			}
		};
		struct crosschain_withdraw_without_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			//transaction_id_type ccw_trx_id;
			vector<transaction_id_type> ccw_trx_ids;
			fc::variant_object withdraw_source_trx;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
			address miner_address;
			asset fee;
			string asset_symbol;
			asset_id_type asset_id;
			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			asset crosschain_fee;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};
		struct crosschain_withdraw_with_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			transaction_id_type ccw_trx_id;
			fc::variant_object withdraw_source_trx;
			//TODO:refund balance in the situation that channel account tie to formal account
			guard_member_id_type sign_guard;
			string asset_symbol;
			asset fee;
			address guard_address;
			string ccw_trx_signature;

			address fee_payer()const {
				return guard_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, guard_address, 1));
			}
		};
		struct crosschain_withdraw_combine_sign_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			fc::variant_object cross_chain_trx;
			vector<transaction_id_type> signed_trx_ids;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
			transaction_id_type withdraw_trx;
			address miner_address;
			string asset_symbol;
			asset fee;
			string crosschain_trx_id;
			address fee_payer()const {
				return miner_address;
			}
			asset crosschain_fee;
			optional<asset> get_fee()const { return crosschain_fee; }
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
			}
		};

		struct crosschain_withdraw_result_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			crosschain::hd_trx cross_chain_trx;
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
	}
}
FC_REFLECT(graphene::chain::crosschain_record_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_record_operation, (fee)(cross_chain_trx)(miner_broadcast)(asset_id)(asset_symbol)(miner_address))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation,(fee)(withdraw_account)(amount)(asset_symbol)(asset_id)(crosschain_account)(memo))
FC_REFLECT(graphene::chain::crosschain_withdraw_without_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_without_sign_operation,(fee)(ccw_trx_ids)(withdraw_source_trx)(miner_broadcast)(miner_address)(asset_id)(asset_symbol)(crosschain_fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_with_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_with_sign_operation,(fee)(ccw_trx_id)(asset_symbol)(withdraw_source_trx)(sign_guard)(guard_address)(ccw_trx_signature))
FC_REFLECT(graphene::chain::crosschain_withdraw_combine_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_combine_sign_operation, (fee)(cross_chain_trx)(crosschain_trx_id)(signed_trx_ids)(miner_broadcast)(withdraw_trx)(miner_address)(asset_symbol))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation, (fee)(cross_chain_trx)(miner_broadcast)(miner_address))
