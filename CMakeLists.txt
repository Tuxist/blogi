cmake_minimum_required(VERSION 3.18)

project(blogi)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(libnetplus REQUIRED)
find_package(libcryptplus REQUIRED)
find_package(htmlpp REQUIRED)
find_package(httppp REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenLDAP)
find_package(OpenSSL REQUIRED)
find_package(Hiredis REQUIRED)

include_directories(
    ${NETPLUS_INCLUDE_DIRS}
    ${CRYPTPLUS_INCLUDE_DIRS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${OPENLDAP_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

if(${OPENLDAP_FOUND})
    add_compile_definitions(LDAPSUPPORT=TRUE)
endif()

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tests)
