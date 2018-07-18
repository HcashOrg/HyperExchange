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
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
			void            validate()const;
			//share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, pay_back_owner, 1));
			}
		};

		struct bonus_operation :public base_operation {
			struct fee_parameters_type {
				uint64_t fee = 20 * GRAPHENE_BLOCKCHAIN_PRECISION;
			};
			address bonus_owner;
			asset fee;
			map<std::string, share_type> bonus_balance;
			address fee_payer()const {
				return bonus_owner;
			}
			optional<guarantee_object_id_type> guarantee_id;
			optional<guarantee_object_id_type> get_guarantee_id()const { return guarantee_id; }
			void            validate()const;
			//share_type      calculate_fee(const fee_parameters_type& k)const;
			void get_required_authorities(vector<authority>& a)const {
				a.push_back(authority(1, bonus_owner, 1));
			}
		};

	}
}
FC_REFLECT(graphene::chain::pay_back_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::pay_back_operation, (pay_back_owner)(pay_back_balance)(guarantee_id)(fee))
FC_REFLECT(graphene::chain::bonus_operation::fee_parameters_type, (fee))
FC_REFLECT(graphene::chain::bonus_operation, (bonus_owner)(bonus_balance)(guarantee_id)(fee))

