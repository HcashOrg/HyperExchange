add_executable( key_decryptor main.cpp )
if( UNIX AND NOT APPLE )
  set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling key_decryptor with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()
IF(WIN32)
 target_link_libraries( key_decryptor		  
                        PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet fc )
ELSE()
target_link_libraries( key_decryptor
                       PRIVATE graphene_app graphene_net graphene_chain graphene_egenesis_brief graphene_utilities graphene_wallet fc crosschain_privatekey_management  $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
ENDIF()

if(MSVC)
  set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   key_decryptor

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
