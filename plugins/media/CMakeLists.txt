add_library(media SHARED media.cpp)

target_link_libraries(media dl blogidev)

install (TARGETS media  DESTINATION lib/blogi/plugins)
