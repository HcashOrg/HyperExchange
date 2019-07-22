file(GLOB HEADERS "include/graphene/db/*.hpp")
add_library( graphene_db undo_database.cpp index.cpp object_database.cpp serializable_undo_state.cpp ${HEADERS} )
target_link_libraries( graphene_db fc leveldb)
target_include_directories( graphene_db PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_SOURCE_DIR}/../chain/include" "${CMAKE_CURRENT_SOURCE_DIR}/../crosschain/include" "${CMAKE_CURRENT_SOURCE_DIR}/../uvm/include" "${CMAKE_CURRENT_SOURCE_DIR}/../uvm/vmgc/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../db/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../crosschain/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../chain/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../jsondiff/jsondiff-cpp/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../utilities/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../wallet/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../crosschain_privatekey_management/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../leveldb/include"
"${CMAKE_CURRENT_SOURCE_DIR}/../uvm/vmgc/include"
 )
if(MSVC)
  set_source_files_properties( undo_database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)
install( TARGETS
   graphene_db

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
install( FILES ${HEADERS} DESTINATION "include/graphene/db" )
