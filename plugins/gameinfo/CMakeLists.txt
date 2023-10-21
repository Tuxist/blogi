add_library(gamestatus SHARED gameinfo.cpp)

target_link_libraries(gamestatus dl blogidev gameinfoplus)

install (TARGETS gamestatus  DESTINATION lib/blogi/plugins)
