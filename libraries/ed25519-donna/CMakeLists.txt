cmake_minimum_required(VERSION 3.5)

project(ed25519-donna)

set(CMAKE_CXX_STANDARD 11)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "-std=c++11 -D_DARWIN_C_SOURCE -Wno-invalid-partial-specialization")
endif()

file(GLOB HEADERS "ed25519.h" "./hmac/hmac_sha2.h" "./hmac/sha2.h" "bech32.h" "./bech32/segwit_addr.h" "bytomlib.hpp" "./SHA3IUF/*.h")

set(SOURCES 
	ed25519.c
	./hmac/hmac_sha2.c
	./hmac/sha2.c
	./bech32/segwit_addr.cpp
	./SHA3IUF/sha3.cpp
	bytomlib.cpp
	#bech32.cpp
)
find_package(OpenSSL REQUIRED)

add_library( ed25519-donna ${SOURCES} ${HEADERS} )

target_link_libraries( ed25519-donna 
  PUBLIC fc)
  
add_executable( ed25519_exe main.cpp)
target_include_directories(ed25519_exe PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE  ${BOOST_ROOT}/)
target_link_libraries(ed25519_exe PRIVATE ed25519-donna  ${Boost_LIBRARIES})

#add_library( ed25519_exe ${SOURCES} ${HEADERS} )


if (USE_PCH)
  set_target_properties(ed25519-donna PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(ed25519-donna)
endif(USE_PCH)

install( TARGETS
   ed25519-donna

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

