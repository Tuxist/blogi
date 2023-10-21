add_library(staticpage SHARED static.cpp)

target_link_libraries(staticpage dl blogidev)

install (TARGETS staticpage  DESTINATION lib/blogi/plugins)
