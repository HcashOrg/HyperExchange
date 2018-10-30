file(GLOB HEADERS "include/graphene/chain/*.hpp" "../uvm/include/uvm/*.hpp" "../jsondiff/jsondiff-cpp/include/jsondiff/*.h"  "$ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/secp256k1/include/*.h" )
file(GLOB PROTOCOL_HEADERS "include/graphene/chain/protocol/*.hpp")

if( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        db_balance.cpp
        db_block.cpp
        db_debug.cpp
        db_getter.cpp
        db_init.cpp
        db_maint.cpp
        db_management.cpp
        db_market.cpp
        db_update.cpp
        db_witness_schedule.cpp
		db_lock_balance.cpp
	db_crosschain_trx.cpp
	db_coldhot_trx.cpp
	db_pay_back.cpp
	db_eths_trx.cpp
      )
   message( STATUS "Graphene database unity build disabled" )
else( GRAPHENE_DISABLE_UNITY_BUILD )
   set( GRAPHENE_DB_FILES
        database.cpp )
   message( STATUS "Graphene database unity build enabled" )
endif( GRAPHENE_DISABLE_UNITY_BUILD )

## SORT .cpp by most likely to change / break compile
add_library( graphene_chain

             # As database takes the longest to compile, start it first
             ${GRAPHENE_DB_FILES}
             fork_database.cpp
			 protocol/lockbalance.cpp
			 protocol/guard_lock_balance.cpp
			 protocol/guard_refund_balance.cpp
             protocol/types.cpp
             protocol/address.cpp
             protocol/authority.cpp
             protocol/asset.cpp
             protocol/assert.cpp
             protocol/account.cpp
             protocol/transfer.cpp
             protocol/committee_member.cpp
             protocol/witness.cpp
             protocol/market.cpp
             protocol/proposal.cpp
			 protocol/referendum.cpp
             protocol/withdraw_permission.cpp
             protocol/asset_ops.cpp
             protocol/memo.cpp
             protocol/worker.cpp
             protocol/custom.cpp
             protocol/operations.cpp
             protocol/transaction.cpp
             protocol/block.cpp
             protocol/fee_schedule.cpp
             protocol/confidential.cpp
             protocol/vote.cpp
			eth_seri_record_evaluate.cpp
			eth_seri_record.cpp
             genesis_state.cpp
             get_config.cpp
	     pts_address.cpp
	     crosschain_record.cpp
	     crosschain_record_evaluate.cpp
             account_object.cpp
             evaluator.cpp
             balance_evaluator.cpp
	     pay_back.cpp
	     pay_back_evaluator.cpp
             account_evaluator.cpp
             assert_evaluator.cpp
             witness_evaluator.cpp
             committee_member_evaluator.cpp
             asset_evaluator.cpp
             transfer_evaluator.cpp
             proposal_evaluator.cpp
			 referendum_evaluator.cpp
             market_evaluator.cpp
             coldhot_transfer_evaluate.cpp
             vesting_balance_evaluator.cpp
             withdraw_permission_evaluator.cpp
             worker_evaluator.cpp
             confidential_evaluator.cpp
             special_authority.cpp
             buyback.cpp
             coldhot_withdraw.cpp
 	     guard_lock_balance_evaluator.cpp
	     guard_refund_balance_evaluator.cpp
	     lockbalance_evaluator.cpp
             asset_object.cpp
             fba_object.cpp
             proposal_object.cpp
			 referendum_object.cpp
             vesting_balance_object.cpp
             block_database.cpp

             is_authorized_asset.cpp

             contract.cpp
             storage.cpp
             contract_engine.cpp
             contract_engine_builder.cpp
             uvm_contract_engine.cpp
             contract_evaluate.cpp
             contract_entry.cpp
             uvm_chain_api.cpp
             db_contract_trx.cpp
             native_contract.cpp
             Keccak.cpp

	     crosschain_record_evaluate.cpp
             ${HEADERS}
             ${PROTOCOL_HEADERS}
           )
target_link_libraries( graphene_chain fc graphene_db crosschain uvm jsondiff)
target_include_directories( graphene_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../jsondiff/jsondiff-cpp/include" "$ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/secp256k1/include/*.h" )

if(MSVC)
  set_source_files_properties( db_init.cpp db_block.cpp database.cpp block_database.cpp fork_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   graphene_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/chain" )
INSTALL( FILES ${PROTOCOL_HEADERS} DESTINATION "include/graphene/chain/protocol" )
