file(GLOB HEADERS "include/graphene/account_history/*.hpp")

add_library( graphene_transaction 
             transaction_plugin.cpp
           )

target_link_libraries( graphene_transaction graphene_chain graphene_app )
target_include_directories( graphene_transaction 
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( transaction_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_transaction

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/transaction" )

