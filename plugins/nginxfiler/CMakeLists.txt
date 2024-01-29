find_package(json-c REQUIRED)

add_library(nginxfiler SHARED filer.cpp)

target_link_libraries(nginxfiler dl blogidev json-c::json-c)

install (TARGETS nginxfiler  DESTINATION lib/blogi/plugins)
