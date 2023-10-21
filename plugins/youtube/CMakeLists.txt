add_library(youtube SHARED youtube.cpp)

target_link_libraries(youtube dl blogidev)

install (TARGETS youtube  DESTINATION lib/blogi/plugins)
