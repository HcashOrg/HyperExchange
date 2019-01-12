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

#include <fc/smart_ref_impl.hpp>

#include <graphene/chain/account_evaluator.hpp>
#include <graphene/chain/buyback.hpp>
#include <graphene/chain/buyback_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/internal_exceptions.hpp>
#include <graphene/chain/special_authority.hpp>
#include <graphene/chain/special_authority_object.hpp>
#include <graphene/chain/worker_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/crosschain_privatekey_management/util.hpp>
#include <graphene/chain/balance_object.hpp>
#include <algorithm>

namespace graphene { namespace chain {

void verify_authority_accounts( const database& db, const authority& a )
{
   const auto& chain_params = db.get_global_properties().parameters;
   GRAPHENE_ASSERT(
      a.num_auths() <= chain_params.maximum_authority_membership,
      internal_verify_auth_max_auth_exceeded,
      "Maximum authority membership exceeded" );
   for( const auto& acnt : a.account_auths )
   {
      GRAPHENE_ASSERT( db.find_object( acnt.first ) != nullptr,
         internal_verify_auth_account_not_found,
         "Account ${a} specified in authority does not exist",
         ("a", acnt.first) );
   }
}

void verify_account_votes( const database& db, const account_options& options )
{
   // ensure account's votes satisfy requirements
   // NB only the part of vote checking that requires chain state is here,
   // the rest occurs in account_options::validate()

   const auto& gpo = db.get_global_properties();
   const auto& chain_params = gpo.parameters;

   FC_ASSERT( options.num_witness <= chain_params.maximum_miner_count,
              "Voted for more witnesses than currently allowed (${c})", ("c", chain_params.maximum_miner_count) );
   FC_ASSERT( options.num_committee <= chain_params.maximum_guard_count,
              "Voted for more committee members than currently allowed (${c})", ("c", chain_params.maximum_guard_count) );

   uint32_t max_vote_id = gpo.next_available_vote_id;
   bool has_worker_votes = false;
   for( auto id : options.votes )
   {
      FC_ASSERT( id < max_vote_id );
      has_worker_votes |= (id.type() == vote_id_type::worker);
   }

   FC_ASSERT(options.miner_pledge_pay_back >= 0 && options.miner_pledge_pay_back <= 20, "miner_pledge_pay_back must between 0 20");
   const auto& against_worker_idx = db.get_index_type<worker_index>().indices().get<by_vote_against>();
   for (auto id : options.votes)
   {
	   if (id.type() == vote_id_type::worker)
	   {
		   FC_ASSERT(against_worker_idx.find(id) == against_worker_idx.end());
	   }
   }

}


void_result account_create_evaluator::do_evaluate( const account_create_operation& op )
{ try {
   database& d = db();
   try
   {
      verify_authority_accounts( d, op.owner );
      verify_authority_accounts( d, op.active );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_create_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_create_auth_account_not_found )

   if( op.extensions.value.owner_special_authority.valid() )
      evaluate_special_authority( d, *op.extensions.value.owner_special_authority );
   if( op.extensions.value.active_special_authority.valid() )
      evaluate_special_authority( d, *op.extensions.value.active_special_authority );
   if( op.extensions.value.buyback_options.valid() )
      evaluate_buyback_account_options( d, *op.extensions.value.buyback_options );

   auto& acnt_indx = d.get_index_type<account_index>();
   if( op.name.size() )
   {
      auto current_account_itr = acnt_indx.indices().get<by_name>().find( op.name );
      FC_ASSERT( current_account_itr == acnt_indx.indices().get<by_name>().end() );
   }
   if (d.head_block_num() > ACCOUN_EVALUATE_150000)
   {
	   auto addr = address(op.owner.get_keys().front());
	   auto addr_itr = acnt_indx.indices().get<by_address>().find(addr);
	   FC_ASSERT(addr_itr == acnt_indx.indices().get<by_address>().end(),"there an account with same address.");
   }
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type account_create_evaluator::do_apply( const account_create_operation& o )
{ try {

   database& d = db();
   const auto& new_acnt_object = db().create<account_object>( [&]( account_object& obj ){
         obj.registrar = o.registrar;
         obj.referrer = o.referrer;
         obj.lifetime_referrer = o.referrer(db()).lifetime_referrer;

         auto& params = db().get_global_properties().parameters;
         obj.network_fee_percentage = params.network_percent_of_fee;
         obj.lifetime_referrer_fee_percentage = params.lifetime_referrer_percent_of_fee;
         obj.referrer_rewards_percentage = 0;

         obj.name             = o.name;
         obj.owner            = o.owner;
         obj.active           = o.active;
         obj.options          = o.options;
		 obj.addr             = o.owner.get_keys().front();
         obj.statistics = db().create<account_statistics_object>([&](account_statistics_object& s){s.owner = obj.id;}).id;

         if( o.extensions.value.owner_special_authority.valid() )
            obj.owner_special_authority = *(o.extensions.value.owner_special_authority);
         if( o.extensions.value.active_special_authority.valid() )
            obj.active_special_authority = *(o.extensions.value.active_special_authority);
         if( o.extensions.value.buyback_options.valid() )
         {
            obj.allowed_assets = o.extensions.value.buyback_options->markets;
            obj.allowed_assets->emplace( o.extensions.value.buyback_options->asset_to_buy );
         }
   });

 
   const auto& dynamic_properties = db().get_dynamic_global_properties();
   db().modify(dynamic_properties, [](dynamic_global_property_object& p) {
      ++p.accounts_registered_this_interval;
   });

   const auto& global_properties = db().get_global_properties();
   if( dynamic_properties.accounts_registered_this_interval %
       global_properties.parameters.accounts_per_fee_scale == 0 )
      db().modify(global_properties, [&dynamic_properties](global_property_object& p) {
         p.parameters.current_fees->get<account_create_operation>().basic_fee <<= p.parameters.account_fee_scale_bitshifts;
      });

   if(    o.extensions.value.owner_special_authority.valid()
       || o.extensions.value.active_special_authority.valid() )
   {
      db().create< special_authority_object >( [&]( special_authority_object& sa )
      {
         sa.account = new_acnt_object.id;
      } );
   }

   if( o.extensions.value.buyback_options.valid() )
   {
      asset_id_type asset_to_buy = o.extensions.value.buyback_options->asset_to_buy;

      d.create< buyback_object >( [&]( buyback_object& bo )
      {
         bo.asset_to_buy = asset_to_buy;
      } );

      d.modify( asset_to_buy(d), [&]( asset_object& a )
      {
         a.buyback_account = new_acnt_object.id;
      } );
   }

   return new_acnt_object.id;
} FC_CAPTURE_AND_RETHROW((o)) }


void_result account_update_evaluator::do_evaluate( const account_update_operation& o )
{ try {
   database& d = db();
   try
   {
      if( o.owner )  verify_authority_accounts( d, *o.owner );
      if( o.active ) verify_authority_accounts( d, *o.active );
   }
   GRAPHENE_RECODE_EXC( internal_verify_auth_max_auth_exceeded, account_update_max_auth_exceeded )
   GRAPHENE_RECODE_EXC( internal_verify_auth_account_not_found, account_update_auth_account_not_found )

   if( o.extensions.value.owner_special_authority.valid() )
      evaluate_special_authority( d, *o.extensions.value.owner_special_authority );
   if( o.extensions.value.active_special_authority.valid() )
      evaluate_special_authority( d, *o.extensions.value.active_special_authority );

   acnt = &o.account(d);
   FC_ASSERT(acnt->addr==o.addr);
   if( o.new_options.valid() )
      verify_account_votes( d, *o.new_options );

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

bool account_update_evaluator::if_evluate()
{
	return true;
}

void_result account_update_evaluator::do_apply( const account_update_operation& o )
{ try {
   database& d = db();
   bool sa_before, sa_after;
   d.modify( *acnt, [&](account_object& a){
      if( o.owner )
      {
         a.owner = *o.owner;
         a.top_n_control_flags = 0;
      }
      if( o.active )
      {
         a.active = *o.active;
         a.top_n_control_flags = 0;
      }
      if( o.new_options ) a.options = *o.new_options;
      sa_before = a.has_special_authority();
      if( o.extensions.value.owner_special_authority.valid() )
      {
         a.owner_special_authority = *(o.extensions.value.owner_special_authority);
         a.top_n_control_flags = 0;
      }
      if( o.extensions.value.active_special_authority.valid() )
      {
         a.active_special_authority = *(o.extensions.value.active_special_authority);
         a.top_n_control_flags = 0;
      }
      sa_after = a.has_special_authority();
   });

   if( sa_before & (!sa_after) )
   {
      const auto& sa_idx = d.get_index_type< special_authority_index >().indices().get<by_account>();
      auto sa_it = sa_idx.find( o.account );
      assert( sa_it != sa_idx.end() );
      d.remove( *sa_it );
   }
   else if( (!sa_before) & sa_after )
   {
      d.create< special_authority_object >( [&]( special_authority_object& sa )
      {
         sa.account = o.account;
      } );
   }

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_whitelist_evaluator::do_evaluate(const account_whitelist_operation& o)
{ try {
   database& d = db();

   listed_account = &o.account_to_list(d);
   if( !d.get_global_properties().parameters.allow_non_member_whitelists )
      FC_ASSERT(o.authorizing_account(d).is_lifetime_member());

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_whitelist_evaluator::do_apply(const account_whitelist_operation& o)
{ try {
   database& d = db();

   d.modify(*listed_account, [&o](account_object& a) {
      if( o.new_listing & o.white_listed )
         a.whitelisting_accounts.insert(o.authorizing_account);
      else
         a.whitelisting_accounts.erase(o.authorizing_account);

      if( o.new_listing & o.black_listed )
         a.blacklisting_accounts.insert(o.authorizing_account);
      else
         a.blacklisting_accounts.erase(o.authorizing_account);
   });

   /** for tracking purposes only, this state is not needed to evaluate */
   d.modify( o.authorizing_account(d), [&]( account_object& a ) {
     if( o.new_listing & o.white_listed )
        a.whitelisted_accounts.insert( o.account_to_list );
     else
        a.whitelisted_accounts.erase( o.account_to_list );

     if( o.new_listing & o.black_listed )
        a.blacklisted_accounts.insert( o.account_to_list );
     else
        a.blacklisted_accounts.erase( o.account_to_list );
   });

   return void_result();
} FC_CAPTURE_AND_RETHROW( (o) ) }

void_result account_upgrade_evaluator::do_evaluate(const account_upgrade_evaluator::operation_type& o)
{ try {
   database& d = db();

   account = &d.get(o.account_to_upgrade);
   FC_ASSERT(!account->is_lifetime_member());

   return {};
} FC_RETHROW_EXCEPTIONS( error, "Unable to upgrade account '${a}'", ("a",o.account_to_upgrade(db()).name) ) }

void_result account_upgrade_evaluator::do_apply(const account_upgrade_evaluator::operation_type& o)
{ try {
   database& d = db();

   d.modify(*account, [&](account_object& a) {
      if( o.upgrade_to_lifetime_member )
      {
         // Upgrade to lifetime member. I don't care what the account was before.
         a.statistics(d).process_fees(a, d);
         a.membership_expiration_date = time_point_sec::maximum();
         a.referrer = a.registrar = a.lifetime_referrer = a.get_id();
         a.lifetime_referrer_fee_percentage = GRAPHENE_100_PERCENT - a.network_fee_percentage;
      } else if( a.is_annual_member(d.head_block_time()) ) {
         // Renew an annual subscription that's still in effect.
         FC_ASSERT(a.membership_expiration_date - d.head_block_time() < fc::days(3650),
                   "May not extend annual membership more than a decade into the future.");
         a.membership_expiration_date += fc::days(365);
      } else {
         // Upgrade from basic account.
         a.statistics(d).process_fees(a, d);
         assert(a.is_basic_account(d.head_block_time()));
         a.referrer = a.get_id();
         a.membership_expiration_date = d.head_block_time() + fc::days(365);
      }
   });

   return {};
} FC_RETHROW_EXCEPTIONS( error, "Unable to upgrade account '${a}'", ("a",o.account_to_upgrade(db()).name) ) }

void_result account_bind_evaluator::do_evaluate(const account_bind_operation& o)
{ try {
	// One-to-one binding between link account and tunnel address.
	auto& instance = graphene::crosschain::crosschain_manager::get_instance();
	if (!instance.contain_crosschain_handles(o.crosschain_type))
		return void_result();
	auto crosschain_interface = instance.get_crosschain_handle(o.crosschain_type);
	if (!crosschain_interface->valid_config())
		return void_result();
	if (fc::time_point::now() <= db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
		FC_ASSERT(crosschain_interface->validate_signature(o.tunnel_address, o.tunnel_address, o.tunnel_signature));
	auto &bind_idx = db().get_index_type<account_binding_index>().indices().get<by_account_binding>();
	auto bind_itr = bind_idx.find(boost::make_tuple(o.addr, o.crosschain_type));
	FC_ASSERT(bind_itr == bind_idx.end());

	auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
	auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.tunnel_address, o.crosschain_type));
	FC_ASSERT(tunnel_itr == tunnel_idx.end());

	return void_result();
} FC_CAPTURE_AND_RETHROW((o))
}

object_id_type account_bind_evaluator::do_apply(const account_bind_operation& o)
{ try {
	database& d = db();
	const auto & binding = d.create<account_binding_object>([&](account_binding_object& a) {
		a.owner = o.addr;
		a.bind_account = o.tunnel_address;
		a.chain_type = o.crosschain_type;
	});
	return binding.id;
} FC_CAPTURE_AND_RETHROW((o))
}

void_result account_unbind_evaluator::do_evaluate(const account_unbind_operation& o)
{
	try {
		// One-to-one binding between link account and tunnel address.
		// One-to-one binding between link account and tunnel address.
		auto& instance = graphene::crosschain::crosschain_manager::get_instance();
		if (!instance.contain_crosschain_handles(o.crosschain_type))
			return void_result();
		auto crosschain_interface = instance.get_crosschain_handle(o.crosschain_type);
		if (!crosschain_interface->valid_config())
			return void_result();
		if (fc::time_point::now() <= db().head_block_time() + fc::seconds(db().get_global_properties().parameters.validate_time_period))
			FC_ASSERT(crosschain_interface->validate_signature(o.tunnel_address, o.tunnel_address, o.tunnel_signature));

		auto &bind_idx = db().get_index_type<account_binding_index>().indices().get<by_account_binding>();
		auto bind_itr = bind_idx.find(boost::make_tuple(o.addr, o.crosschain_type));
		FC_ASSERT(bind_itr != bind_idx.end());

		auto &tunnel_idx = db().get_index_type<account_binding_index>().indices().get<by_tunnel_binding>();
		auto tunnel_itr = tunnel_idx.find(boost::make_tuple(o.tunnel_address, o.crosschain_type));
		FC_ASSERT(tunnel_itr != tunnel_idx.end());

		return void_result();
	} FC_CAPTURE_AND_RETHROW((o))
}

object_id_type account_unbind_evaluator::do_apply(const account_unbind_operation& o)
{
	try {
		database& d = db();
		auto &bind_idx = d.get_index_type<account_binding_index>().indices().get<by_account_binding>();
		auto bind_itr = bind_idx.find(boost::make_tuple(o.addr, o.crosschain_type));
		auto id = bind_itr->id;
		d.remove(*bind_itr);
		return id;
	} FC_CAPTURE_AND_RETHROW((o))
}

void_result account_multisig_create_evaluator::do_evaluate(const account_multisig_create_operation& o)
{ try {
	//Check if this address exists.
	const auto &guard_change_idx = db().get_index_type<guard_member_index>().indices().get<by_account>();
	
	//Check if all the signatures are valid.
	const auto &accounts = db().get_index_type<account_index>().indices().get<by_address>();
	auto addr = address(fc::ecc::public_key(o.signature, fc::sha256::hash(o.new_address_hot+o.new_address_cold)));
	FC_ASSERT(accounts.find(addr) != accounts.end());
	FC_ASSERT(o.account_id == accounts.find(addr)->id);
	FC_ASSERT(guard_change_idx.find(o.account_id)->formal == true);
	auto prk_ptr = graphene::privatekey_management::crosschain_management::get_instance().get_crosschain_prk(o.crosschain_type);
	if (prk_ptr == nullptr)
		return void_result();
	auto address_hot = prk_ptr->get_address_by_pubkey(o.new_pubkey_hot);
	auto address_cold = prk_ptr->get_address_by_pubkey(o.new_pubkey_cold);
	FC_ASSERT(address_hot == o.new_address_hot);
	FC_ASSERT(address_cold == o.new_address_cold);
	const auto& multisig_idx = db().get_index_type<multisig_address_index>().indices().get<by_account_chain_type>();
	vector<guard_member_object>guards = db().get_guard_members();
	for (const auto& guard : guards)
	{
		if (guard.guard_member_account == o.account_id)
			continue;
		auto iter = multisig_idx.find(boost::make_tuple(o.crosschain_type, guard.guard_member_account, multisig_account_pair_id_type()));
		if (iter != multisig_idx.end())
		{
			FC_ASSERT(iter->new_address_hot != o.new_address_hot && iter->new_address_hot != o.new_address_cold);
			FC_ASSERT(iter->new_address_cold != o.new_address_cold && iter->new_address_cold != o.new_address_hot);
		}
	}
	return void_result();
} FC_CAPTURE_AND_RETHROW((o))
}

void_result account_multisig_create_evaluator::do_apply(const account_multisig_create_operation& o)
{ try {
	database& d = db();
	const auto & binding = d.create<multisig_address_object>([&](multisig_address_object& a) {
		a.guard_account = o.account_id;
		a.chain_type = o.crosschain_type;
		a.signature = o.signature;
		a.new_address_hot = o.new_address_hot;
		a.new_pubkey_hot = o.new_pubkey_hot;
		a.new_address_cold = o.new_address_cold;
		a.new_pubkey_cold = o.new_pubkey_cold;
	});
	return void_result();
} FC_CAPTURE_AND_RETHROW((o))
}

void_result account_create_multisignature_address_evaluator::do_evaluate(const account_create_multisignature_address_operation& o)
{
	try {
		const database& d = db();
		FC_ASSERT(o.pubs.size() <= 15 && o.pubs.size() > 1);
		FC_ASSERT(o.required <= o.pubs.size() && o.required >0);
		auto& bal_idx =d.get_index_type<balance_index>();
		const auto& by_owner_idx = bal_idx.indices().get<by_owner>();
		auto itr = by_owner_idx.find(boost::make_tuple(o.multisignature, asset_id_type()));
		if (itr != by_owner_idx.end())
			FC_ASSERT(!itr->multisignatures.valid());
		auto pubkey = fc::ecc::public_key();
		for (auto iter :o.pubs)
		{
			auto temp = iter.operator fc::ecc::public_key();
			if (!pubkey.valid())
			{
				pubkey = temp;
			}
			pubkey = pubkey.add(fc::sha256::hash(temp));
		}
		pubkey = pubkey.add(fc::sha256::hash(o.required));
		FC_ASSERT(o.multisignature == address(pubkey,addressVersion::MULTISIG));
		return void_result();
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result account_create_multisignature_address_evaluator::do_apply(const account_create_multisignature_address_operation& o)
{
	try {
		database& d = db();
		std::map<int, fc::flat_set<public_key_type>> temp;
		temp[o.required] = o.pubs;
		auto& bal_idx = d.get_index_type<balance_index>();
		auto& by_owner_idx = bal_idx.indices().get<by_owner>();
		auto itr = by_owner_idx.find(boost::make_tuple(o.multisignature, asset_id_type()));
		if (itr != by_owner_idx.end())
		{
			d.modify(*itr, [&](balance_object& obj) {
				obj.multisignatures = temp;
			});
		}
		else
		{
			d.create<balance_object>([&](balance_object& obj) {
				obj.balance = asset();
				obj.owner = o.multisignature;
				obj.multisignatures = temp;
			});
		}
		
		return void_result();
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result block_address_evaluator::do_evaluate(const block_address_operation& o)
{
	try {
		FC_ASSERT(o.blocked_address.size() >0 );
		const auto& blocked_idx =db().get_index_type<blocked_index>().indices().get<by_address>();
		for (auto addr : o.blocked_address)
		{
			FC_ASSERT(blocked_idx.find(addr) == blocked_idx.end(),"address has been blocked.");
		}
	}FC_CAPTURE_AND_RETHROW((o))
}
void_result block_address_evaluator::do_apply(const block_address_operation& o)
{
	try {
		for (auto addr : o.blocked_address)
		{
			db().create<blocked_address_object>([&](blocked_address_object& obj) {
				obj.blocked_address = addr;
			});
		}
		
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result cancel_address_block_evaluator::do_evaluate(const cancel_address_block_operation& o)
{
	try {
		FC_ASSERT(o.cancel_blocked_address.size() > 0);
		const auto& blocked_idx = db().get_index_type<blocked_index>().indices().get<by_address>();
		for (auto addr : o.cancel_blocked_address)
		{
			FC_ASSERT(blocked_idx.find(addr) != blocked_idx.end(), "address has not been blocked.");
		}
	}FC_CAPTURE_AND_RETHROW((o))
}

void_result cancel_address_block_evaluator::do_apply(const cancel_address_block_operation& o)
{
	try {
		FC_ASSERT(o.cancel_blocked_address.size() > 0);
		auto& blocked_idx = db().get_index_type<blocked_index>().indices().get<by_address>();
		for (auto addr : o.cancel_blocked_address)
		{
			db().remove(*blocked_idx.find(addr));
		}
	}FC_CAPTURE_AND_RETHROW((o))
}



} } // graphene::chain
