find_package(gameinfoplus)

if(${gameinfoplus_FOUND})
    add_library(gamestatus SHARED gameinfo.cpp)

    target_include_directories(gamestatus PUBLIC ${gameinfoplus_INCLUDE_DIR})
    target_link_libraries(gamestatus dl blogidev  ${gameinfoplus_LIBRARIES})

    install (TARGETS gamestatus  DESTINATION lib/blogi/plugins)
endif()
