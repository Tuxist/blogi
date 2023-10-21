add_library(content SHARED content.cpp)

target_link_libraries(content dl blogidev)

install (TARGETS content DESTINATION lib/blogi/plugins)
