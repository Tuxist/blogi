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
    htmlpp
    httppp
    cryptplus
    netplus
    uuid
    secureid
    cmdplus
    ${PostgreSQL_LIBRARIES}
    ${SQLite3_LIBRARIES}
    ${OPENLDAP_LIBRARIES}
    ${BROTLI_LIBRARIES}
    confplus
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
    asan
)

install(
    TARGETS blogi  DESTINATION bin
)

