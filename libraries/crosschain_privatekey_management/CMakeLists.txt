project( crosschain_privatekey_management )
cmake_minimum_required( VERSION 2.8.12 )

message(STATUS "Configuring project crosschain_privatekey_management located in: ${CMAKE_CURRENT_SOURCE_DIR}")


find_package(Boost REQUIRED COMPONENTS thread date_time system filesystem program_options signals serialization chrono unit_test_framework context locale iostreams regex)

if (Boost_FOUND)
	message(STATUS "found boost library!")
else (Boost_FOUND)
	message(STATUS "cannot found Boost library!")
endif()


# build library crosschain_privatekey_management
message(STATUS "build library crosschain_privatekey_management")

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp   ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/secp256k1/include $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libbitcoin/include $ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/ )
message(STATUS "HEADERS: ${HEADERS}")

set(sources
   private_key.cpp
   database_privatekey.cpp
   util.cpp
   
   ${HEADERS}
   )

set(BOOST_INCLUDE_DIR $ENV{BOOST_ROOT})
#set(THIRD_LIB $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT})
#message(STATUS "third_lib is"${THIRD_LIB})

add_library( crosschain_privatekey_management
             ${sources}
             ${HEADERS} )

IF(WIN32)
target_compile_definitions( crosschain_privatekey_management PUBLIC  
                            BC_STATIC
							ENABLE_MODULE_RECOVERY
							ENABLE_MODULE_ECDH
							USE_SCALAR_8X32
							USE_FIELD_10X26
							USE_ENDOMORPHISM
							USE_NUM_NONE
							USE_FIELD_INV_BUILTIN
							USE_SCALAR_INV_BUILTIN
							UNICODE
							_UNICODE
							WIN32
							_WIN32
							WIN64
							_WIN64   
							)
ELSE()
target_compile_definitions( crosschain_privatekey_management PUBLIC  
                            BC_STATIC
                                                        ENABLE_MODULE_RECOVERY
                                                        ENABLE_MODULE_ECDH
                                                        USE_SCALAR_8X32
                                                        USE_FIELD_10X26
                                                        USE_ENDOMORPHISM
                                                        USE_NUM_NONE
                                                        USE_FIELD_INV_BUILTIN
                                                        USE_SCALAR_INV_BUILTIN
)
ENDIF()
IF(WIN32)

target_link_libraries( crosschain_privatekey_management PUBLIC graphene_chain   $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/third_libs/blocklink_libbitcoin_secp256k1.lib 
                                                                                $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/third_libs/blocklink_libbitcoin.lib
																$ENV{ETH_CROSSCHAIN_PROJECT}/libs/eth_sign.lib
																$ENV{ETH_CROSSCHAIN_PROJECT}/libs/cryptlib.lib
															   )
ELSE()
	target_link_libraries( crosschain_privatekey_management PUBLIC graphene_chain $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a
	$ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/libeth_sign.a
	$ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/cryptopp/libcryptopp.a
	)
ENDIF()
target_include_directories( crosschain_privatekey_management
                            PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include  $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/secp256k1/include $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libbitcoin/include $ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/ PRIVATE ${BOOST_INCLUDE_DIR}/)
	



add_executable( crosschain_privatekey_management_exe main.cpp )
IF(WIN32)
target_link_libraries(crosschain_privatekey_management_exe PRIVATE crosschain_privatekey_management ${Boost_LIBRARIES}
                                                            $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/third_libs/blocklink_libbitcoin_secp256k1.lib 
                                                            $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/third_libs/blocklink_libbitcoin.lib
															$ENV{ETH_CROSSCHAIN_PROJECT}/libs/eth_sign.lib
															$ENV{ETH_CROSSCHAIN_PROJECT}/libs/cryptlib.lib)

ELSE()
 target_link_libraries(crosschain_privatekey_management_exe PRIVATE crosschain_privatekey_management ${Boost_LIBRARIES}
                                                            $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin_secp256k1.a
                                                            $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libblocklink_libbitcoin.a
							    $ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/libeth_sign.a
							    $ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/cryptopp/libcryptopp.a
)

ENDIF()

target_include_directories(crosschain_privatekey_management_exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include  $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/secp256k1/include  $ENV{CROSSCHAIN_PRIVATEKEY_PROJECT}/libbitcoin/include $ENV{ETH_CROSSCHAIN_PROJECT}/eth_sign/ PRIVATE ${BOOST_ROOT}/)

if (USE_PCH)
  set_target_properties(crosschain_privatekey_management PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(crosschain_privatekey_management)
endif(USE_PCH)

install( TARGETS
   crosschain_privatekey_management

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/crosschain_privatekey_management" )
