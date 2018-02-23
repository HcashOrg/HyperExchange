list( APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/libraries/fc/GitVersionGen" )
include( GetGitRevisionDescription )
get_git_head_revision(GIT_REFSPEC GRAPHENE_GIT_REVISION_SHA)
get_git_unix_timestamp(GRAPHENE_GIT_REVISION_UNIX_TIMESTAMP)
git_describe(GRAPHENE_GIT_REVISION_DESCRIPTION --tags)
if(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)
    set(GRAPHENE_GIT_REVISION_DESCRIPTION "unknown")
endif(NOT GRAPHENE_GIT_REVISION_DESCRIPTION)

file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR} *.hpp *.h)

#file(GLOB_RECURSE BTCSRC ${CMAKE_CURRENT_SOURCE_DIR}/btc  *.cpp)

set(sources
   private_key.cpp
   database_privatekey.cpp
#   ${BTCSRC}
   ${HEADERS}
   )

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/git_revision.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp" @ONLY)
list(APPEND sources "${CMAKE_CURRENT_BINARY_DIR}/git_revision.cpp")



add_library( privatekey_management
             ${sources}
             ${HEADERS} )
target_link_libraries( privatekey_management graphene_chain )
target_include_directories( privatekey_management
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )
							

add_executable( privatekey_management_exe_test main.cpp )
target_link_libraries(privatekey_management_exe_test privatekey_management)

if (USE_PCH)
  set_target_properties(privatekey_management PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
  cotire(privatekey_management)
endif(USE_PCH)

install( TARGETS
   privatekey_management

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/privatekey_management" )