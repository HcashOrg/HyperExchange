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
#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/account_object.hpp>

namespace graphene { namespace chain {

class account_create_evaluator : public evaluator<account_create_evaluator>
{
public:
   typedef account_create_operation operation_type;

   void_result do_evaluate( const account_create_operation& o );
   object_id_type do_apply( const account_create_operation& o ) ;
};

class account_update_evaluator : public evaluator<account_update_evaluator>
{
public:
   typedef account_update_operation operation_type;

   void_result do_evaluate( const account_update_operation& o );
   void_result do_apply( const account_update_operation& o );
   bool if_evluate();
   const account_object* acnt;
};

class account_upgrade_evaluator : public evaluator<account_upgrade_evaluator>
{
public:
   typedef account_upgrade_operation operation_type;

   void_result do_evaluate(const operation_type& o);
   void_result do_apply(const operation_type& o);
   bool if_evluate() { return true; }
   const account_object* account;
};

class account_whitelist_evaluator : public evaluator<account_whitelist_evaluator>
{
public:
   typedef account_whitelist_operation operation_type;

   void_result do_evaluate( const account_whitelist_operation& o);
   void_result do_apply( const account_whitelist_operation& o);
   bool if_evluate() { return true; }
   const account_object* listed_account;
};

class account_bind_evaluator : public evaluator<account_bind_evaluator>
{
public:
	typedef account_bind_operation operation_type;

	void_result do_evaluate(const account_bind_operation& o);
	object_id_type do_apply(const account_bind_operation& o);
};

class account_unbind_evaluator : public evaluator<account_unbind_evaluator>
{
public:
	typedef account_unbind_operation operation_type;

	void_result do_evaluate(const account_unbind_operation& o);
	object_id_type do_apply(const account_unbind_operation& o);

	//const account_binding_object *bind_object;
};

class account_multisig_create_evaluator : public evaluator<account_multisig_create_evaluator>
{
public:
	typedef account_multisig_create_operation operation_type;

	void_result do_evaluate(const account_multisig_create_operation& o);
	void_result do_apply(const account_multisig_create_operation& o);
};
class account_create_multisignature_address_evaluator : public evaluator<account_create_multisignature_address_evaluator>
{
public:
	typedef account_create_multisignature_address_operation operation_type;
	void_result do_evaluate(const account_create_multisignature_address_operation& o);
	void_result do_apply(const account_create_multisignature_address_operation& o);
};
class block_address_evaluator :public evaluator<block_address_evaluator>
{
public:
	typedef block_address_operation operation_type;
	void_result do_evaluate(const block_address_operation& o);
	void_result do_apply(const block_address_operation& o);
};

class cancel_address_block_evaluator : public evaluator<cancel_address_block_evaluator>
{
public:
	typedef cancel_address_block_operation operation_type;
	void_result do_evaluate(const cancel_address_block_operation& o);
	void_result do_apply(const cancel_address_block_operation& o);
};

class add_whiteOperation_list_evaluator : public evaluator<add_whiteOperation_list_evaluator>
{
public:
	typedef add_whiteOperation_list_operation operation_type;
	void_result do_evaluate(const add_whiteOperation_list_operation& o);
	void_result do_apply(const add_whiteOperation_list_operation& o);
};

class cancel_whiteOperation_list_evaluator : public evaluator<cancel_whiteOperation_list_evaluator>
{
public:
	typedef cancel_whiteOperation_list_operation operation_type;
	void_result do_evaluate(const cancel_whiteOperation_list_operation& o);
	void_result do_apply(const cancel_whiteOperation_list_operation& o);
};

class undertaker_evaluator : public evaluator<undertaker_evaluator>
{
public:
	typedef undertaker_operation operation_type;
	void_result do_evaluate(const undertaker_operation& o);
	void_result do_apply(const undertaker_operation& o);
};
class name_transfer_evaluator : public evaluator<name_transfer_evaluator>
{
public:
	typedef name_transfer_operation operation_type;
	void_result do_evaluate(const name_transfer_operation& o);
	void_result do_apply(const name_transfer_operation& o);
};

} } // graphene::chain
