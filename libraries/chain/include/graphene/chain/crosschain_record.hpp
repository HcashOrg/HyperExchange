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
			share_type amount;
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
			transaction_id_type ccw_trx_id;
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
			miner_id_type miner_broadcast;
			string asset_symbol;
			asset fee;
			address miner_address;
			string ccw_trx_signature;

			address fee_payer()const {
				return miner_address;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, miner_address, 1));
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
FC_REFLECT(graphene::chain::crosschain_record_operation, (cross_chain_trx)(fee)(miner_broadcast)(asset_id)(miner_address))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation,(withdraw_account)(amount)(asset_symbol)(fee)(asset_id)(crosschain_account)(memo))
FC_REFLECT(graphene::chain::crosschain_withdraw_without_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_without_sign_operation, (ccw_trx_id)(withdraw_source_trx)(fee)(miner_broadcast)(miner_address)(asset_id)(asset_symbol))
FC_REFLECT(graphene::chain::crosschain_withdraw_with_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_with_sign_operation, (ccw_trx_id)(asset_symbol)(fee)(withdraw_source_trx)(miner_broadcast)(miner_address)(ccw_trx_signature))
FC_REFLECT(graphene::chain::crosschain_withdraw_combine_sign_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_combine_sign_operation, (cross_chain_trx)(signed_trx_ids)(fee)(miner_broadcast)(withdraw_trx)(miner_address))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation, (cross_chain_trx)(miner_broadcast)(fee)(miner_address))