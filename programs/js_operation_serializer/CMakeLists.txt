add_executable( js_operation_serializer main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()
IF(WIN32)
 target_link_libraries( js_operation_serializer  PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_none graphene_utilities graphene_wallet fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ELSE()
target_link_libraries( js_operation_serializer
                       PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_none graphene_utilities graphene_wallet fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ENDIF()


install( TARGETS
   js_operation_serializer

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
