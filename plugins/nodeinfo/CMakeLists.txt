add_library(nodeinfo SHARED nodeinfo.cpp)

target_link_libraries(nodeinfo dl blogidev)

install (TARGETS nodeinfo  DESTINATION lib/blogi/plugins)
