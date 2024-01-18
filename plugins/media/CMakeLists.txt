find_package(Hiredis REQUIRED)

add_library(media SHARED media.cpp redis.cpp)

target_include_directories(media PUBLIC
    ${Hiredis_INCLUDE_DIRS}
)

target_link_libraries(media dl blogidev ${Hiredis_LIBRARIES})

install (TARGETS media  DESTINATION lib/blogi/plugins)
