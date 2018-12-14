add_executable( debug_node main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling debug_node with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()
IF(WIN32)
 target_link_libraries( debug_node		  
                        PRIVATE graphene_app graphene_account_history graphene_market_history graphene_witness graphene_debug_witness graphene_chain graphene_egenesis_full fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ELSE()
target_link_libraries( debug_node
                       PRIVATE graphene_app graphene_account_history graphene_market_history graphene_witness graphene_debug_witness graphene_chain graphene_egenesis_full fc  crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ENDIF()


install( TARGETS
   debug_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
