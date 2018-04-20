#pragma once
#include <graphene/chain/protocol/base.hpp>


namespace graphene {
	namespace chain {
		struct pay_back_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			address pay_back_owner;
			asset fee;
			map<std::string, asset> pay_back_balance;
			address fee_payer()const {
				return pay_back_owner;
			}
			void            validate()const;
			share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, pay_back_owner, 1));
			}
		};
	}
}
FC_REFLECT(graphene::chain::pay_back_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::pay_back_operation, (pay_back_owner)(pay_back_balance)(fee))
