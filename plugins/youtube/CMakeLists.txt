find_package(json-c REQUIRED)

add_library(youtube SHARED youtube.cpp)

target_link_libraries(youtube dl blogidev json-c::json-c)

install (TARGETS youtube  DESTINATION lib/blogi/plugins)
