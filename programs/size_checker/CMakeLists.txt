add_executable( size_checker main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()
IF(WIN32)
 target_link_libraries( size_checker  PRIVATE graphene_chain graphene_egenesis_none fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ELSE()
target_link_libraries( size_checker
                       PRIVATE graphene_chain graphene_egenesis_none fc crosschain_privatekey_management $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} leveldb)
ENDIF()


install( TARGETS
   size_checker

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
