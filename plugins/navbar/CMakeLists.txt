add_library(navbar SHARED navbar.cpp)

target_link_libraries(navbar dl blogidev)

install (TARGETS navbar  DESTINATION lib/blogi/plugins)
