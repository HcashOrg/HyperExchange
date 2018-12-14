
add_executable( genesis_update genesis_update.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()
IF(WIN32)
target_link_libraries( genesis_update PRIVATE graphene_app graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ELSE()
target_link_libraries( genesis_update
                       PRIVATE graphene_app graphene_chain graphene_witness graphene_egenesis_none fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a  ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ENDIF()


install( TARGETS
   genesis_update

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( get_dev_key get_dev_key.cpp )

IF(WIN32)
 target_link_libraries( get_dev_key	PRIVATE graphene_app graphene_chain graphene_egenesis_none graphene_utilities fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
ELSE()
target_link_libraries( get_dev_key
                       PRIVATE graphene_app graphene_chain graphene_egenesis_none graphene_utilities fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
ENDIF()


install( TARGETS
   get_dev_key

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

add_executable( convert_address convert_address.cpp )
IF(WIN32)
target_link_libraries( convert_address	 PRIVATE graphene_chain fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
ELSE()
target_link_libraries( convert_address
                       PRIVATE graphene_chain fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
ENDIF()

