find_path(LIBUUID_INCLUDE_DIR uuid.h PATH_SUFFIXES uuid)
find_library(LIBUUID_LIBRARY libuuid.so)

include_directories(${LIBUUID_INCLUDE_DIR})

add_definitions(
    -DSQLITE_THREADSAFE=2
)

add_library(blogidev STATIC
    session.cpp
    auth.cpp
    database.cpp
    conf.cpp
    plugin.cpp
    theme.cpp
    editor.cpp
    database/pgsql.cpp
    database/sqlite.cpp
)

target_link_libraries(blogidev
    htmlpp::htmlpp
    httppp::httppp
    netplus::netplus
    Upstream::secureid
    cmdplus::cmdplus
    ${LIBUUID_LIBRARY}
    ${PostgreSQL_LIBRARIES}
    ${SQLite3_LIBRARIES}
    ${OPENLDAP_LIBRARIES}
    ${BROTLI_LIBRARIES}
    Upstream::confplus
)

target_compile_options(blogidev PUBLIC "-fPIC")

install(TARGETS blogidev DESTINATION lib/blogi)

add_executable(blogi
    blogi.cpp
)

include_directories(
    ${CMAKE_SOURCE_DIR}/data
    ${CMAKE_SOURCE_DIR}/src/database
    ${BROTLI_INCLUDE_DIRS}
)


#target_include_directories(blogi PUBLIC
#
#)

target_link_libraries(blogi
    stdc++
    blogidev
)

install(
    TARGETS blogi  DESTINATION bin
)

