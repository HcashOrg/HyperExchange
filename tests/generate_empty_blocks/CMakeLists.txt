add_executable( generate_empty_blocks main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()
IF(WIN32)
target_link_libraries( generate_empty_blocks  PRIVATE graphene_app graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ELSE()
target_link_libraries( generate_empty_blocks
                       PRIVATE graphene_app graphene_witness graphene_chain graphene_egenesis_none fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ENDIF()


install( TARGETS
   generate_empty_blocks

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
