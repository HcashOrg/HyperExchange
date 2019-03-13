#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>
#include <graphene/db/generic_index.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <graphene/chain/contract_entry.hpp>
#include <graphene/chain/contract.hpp>
#include <graphene/chain/native_contract.hpp>
#include <graphene/chain/vesting_balance_object.hpp>
#include <cborcpp/cbor.h>

namespace graphene {
	namespace chain {

		// this is native contract for token
		class token_native_contract : public abstract_native_contract
		{
		public:
			static std::string native_contract_key() { return "token"; }

			token_native_contract() {}
			virtual ~token_native_contract() {}

			virtual std::string contract_key() const;
			virtual std::set<std::string> apis() const;
			virtual std::set<std::string> offline_apis() const;
			virtual std::set<std::string> events() const;

			virtual contract_invoke_result invoke(const std::string& api_name, const std::string& api_arg);
			string check_admin();
			string get_storage_state();
			string get_storage_token_name();
			string get_storage_token_symbol();
			int64_t get_storage_supply();
			int64_t get_storage_precision();
			cbor::CborMapValue get_storage_users();
			cbor::CborMapValue get_storage_allowed();
			int64_t get_balance_of_user(const string& owner_addr) const;
			cbor::CborMapValue get_allowed_of_user(const std::string& from_addr) const;
			std::string get_from_address();

			contract_invoke_result init_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result init_token_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result transfer_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result balance_of_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result state_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result token_name_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result token_symbol_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result supply_api(const std::string& api_name, const std::string& api_arg);
			contract_invoke_result precision_api(const std::string& api_name, const std::string& api_arg);
			// ��Ȩ��һ���û����Դ��Լ������������
			// arg format : spenderAddress, amount(with precision)
			contract_invoke_result approve_api(const std::string& api_name, const std::string& api_arg);
			// spender�û�����Ȩ����Ȩ�Ľ���з���ת��
			// arg format : fromAddress, toAddress, amount(with precision)
			contract_invoke_result transfer_from_api(const std::string& api_name, const std::string& api_arg);
			// ��ѯһ���û�������ĳ���û���Ȩ�Ľ��
			// arg format : spenderAddress, authorizerAddress
			contract_invoke_result approved_balance_from_api(const std::string& api_name, const std::string& api_arg);
			// ��ѯ�û���Ȩ�������˵����н��
			// arg format : fromAddress
			contract_invoke_result all_approved_from_user_api(const std::string& api_name, const std::string& api_arg);
			
		};

	}
}

