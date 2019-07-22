/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <graphene/chain/protocol/operations.hpp>

namespace graphene { namespace chain {

uint64_t base_operation::calculate_data_fee( uint64_t bytes, uint64_t price_per_kbyte )
{
   auto result = (fc::uint128(bytes) * price_per_kbyte) / 1024;
   FC_ASSERT( result <= GRAPHENE_MAX_SHARE_SUPPLY );
   return result.to_uint64();
}

void balance_claim_operation::validate()const
{
   FC_ASSERT( fee == asset() );
   FC_ASSERT( balance_owner_key != public_key_type() );
}
void set_balance_operation::validate()const
{
	FC_ASSERT(fee.amount >= 0);
	FC_ASSERT(claimed.asset_id != asset_id_type());
	FC_ASSERT(claimed.amount >= 0);
}

/**
 * @brief Used to validate operations in a polymorphic manner
 */
struct operation_validator
{
   typedef void result_type;
   template<typename T>
   void operator()( const T& v )const { v.validate(); }
};
struct operation_guarantee_idor
{
	typedef optional<guarantee_object_id_type> result_type;
	template<typename T>
	optional<guarantee_object_id_type> operator()(const T& v)const { return v.get_guarantee_id(); }
};
struct operation_fee_payor
{
	typedef fc::variant result_type;
	template<typename T>
	fc::variant operator()(const T& v)const { return fc::variant(v.fee_payer()); }
};
struct operation_get_required_auth
{
   typedef void result_type;

   flat_set<account_id_type>& active;
   flat_set<account_id_type>& owner;
   vector<authority>&         other;


   operation_get_required_auth( flat_set<account_id_type>& a,
     flat_set<account_id_type>& own,
     vector<authority>&  oth ):active(a),owner(own),other(oth){}

   template<typename T>
   void operator()( const T& v )const 
   { 
      //active.insert( v.fee_payer() );
      v.get_required_active_authorities( active ); 
      v.get_required_owner_authorities( owner ); 
      v.get_required_authorities( other );
   }
};
fc::variant operation_fee_payer(const operation& op)
{
	return op.visit(operation_fee_payor());
}
void operation_validate( const operation& op )
{
   op.visit( operation_validator() );
}
bool is_contract_operation(const operation& op)
{
	switch (op.which())
	{
	case operation::tag<chain::contract_register_operation>::value:
		return true;
	case operation::tag<chain::contract_upgrade_operation>::value:
		return true;
	case operation::tag<chain::contract_invoke_operation>::value:
		return true;
	case operation::tag<chain::transfer_contract_operation>::value:
		return true;
	case operation::tag<chain::native_contract_register_operation>::value:
		return true;
	}
	return false;
}
optional<guarantee_object_id_type> operation_gurantee_id(const operation& op)
{
	return op.visit(operation_guarantee_idor());
}


void operation_get_required_authorities( const operation& op, 
                                         flat_set<account_id_type>& active,
                                         flat_set<account_id_type>& owner,
                                         vector<authority>&  other )
{
   op.visit( operation_get_required_auth( active, owner, other ) );
}

} } // namespace graphene::chain
