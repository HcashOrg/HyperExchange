#pragma once

#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/memo.hpp>

#include <uvm/uvm_lib.h>
#include <fc/exception/exception.hpp>


namespace blockchain
{
	namespace contract_engine
	{

		FC_DECLARE_EXCEPTION(uvm_exception, 34000, "uvm error");
		FC_DECLARE_DERIVED_EXCEPTION(uvm_executor_internal_error, blockchain::contract_engine::uvm_exception, 34005, "uvm internal error");
	}
}


namespace uvm {
	namespace blockchain {
		struct Code
		{
			std::set<std::string> abi;
			std::set<std::string> offline_abi;
			std::set<std::string> events;
			std::vector<unsigned char> code;
			std::string code_hash;
			Code() {}
			void SetApis(char* module_apis[], int count, int api_type);
			bool valid() const;
			std::string GetHash() const;
		};
	}
}

namespace graphene {
	namespace chain {

		enum ContractApiType
		{
			chain = 1,
			offline = 2,
			event = 3
		};

		

		struct CodePrintAble
		{
			std::set<std::string> abi;
			std::set<std::string> offline_abi;
			std::set<std::string> events;
			std::map<std::string, std::string> printable_storage_properties;
			std::string printable_code;
			std::string code_hash;

			CodePrintAble() {}

			CodePrintAble(const uvm::blockchain::Code& code);
		};

		typedef uint64_t gas_price_type;
		typedef uint64_t gas_count_type;
	}
}

FC_REFLECT(uvm::blockchain::Code, (abi)(offline_abi)(events)(code)(code_hash));
