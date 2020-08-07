file(GLOB HEADERS "include/graphene/backup/*.hpp")

add_library( graphene_backup
             backup_plugin.cpp
           )

target_link_libraries( graphene_backup graphene_chain graphene_app )
target_include_directories( graphene_backup
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( backup.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   graphene_backup

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/graphene/backup" )

