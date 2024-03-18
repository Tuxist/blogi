cmake_minimum_required(VERSION 3.18)

project(blogi)

SET(CMAKE_CXX_FLAGS "-fPIC -Wall")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(libnetplus REQUIRED)
find_package(libcryptplus REQUIRED)
find_package(htmlpp REQUIRED)
find_package(httppp REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(OpenLDAP)
find_package(Hiredis REQUIRED)
find_package(libsecureid REQUIRED)
find_package(Brotli REQUIRED)

include_directories(
    ${NETPLUS_INCLUDE_DIRS}
    ${CRYPTPLUS_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${SQLite3_INCLUDE_DIRS}
    ${OPENLDAP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
    ${SECUREID_INCLUDE_DIRS}
)

if(${OPENLDAP_FOUND})
    add_compile_definitions(LDAPSUPPORT=TRUE)
endif()

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
add_subdirectory(data)
