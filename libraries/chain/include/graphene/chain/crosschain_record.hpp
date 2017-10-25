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
			string crosschain_account;

			address fee_payer()const {
				return withdraw_account;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, withdraw_account, 1));
			}
		};
		struct crosschain_withdraw_result_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			crosschain::hd_trx cross_chain_trx;
			//TODO:refund balance in the situation that channel account tie to formal account
			miner_id_type miner_broadcast;
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
FC_REFLECT(graphene::chain::crosschain_record_operation, (cross_chain_trx)(miner_broadcast)(miner_address))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation::fee_parameters_type,(fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_operation,(withdraw_account)(amount)(asset_symbol)(crosschain_account))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::crosschain_withdraw_result_operation, (cross_chain_trx)(miner_broadcast)(miner_address))