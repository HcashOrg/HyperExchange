file(GLOB HEADERS "include/graphene/network/*.hpp")

add_library( graphene_network 
             network_heartBeat
           )

target_link_libraries( graphene_network graphene_chain graphene_app )
target_include_directories( graphene_network
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   graphene_network

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
