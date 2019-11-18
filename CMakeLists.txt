# Minimum CMake version required to generate
# our build system
cmake_minimum_required(VERSION 3.0)

# Name of our Project
project(tplink_sms)

FIND_PACKAGE(RapidJSON REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED) # for MD5 hash
FIND_PACKAGE(CURL REQUIRED)

set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)
set (CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS})
# add_executable creates an executable with
# The given name.
add_executable(tplink_sms tplink_sms.cxx)

MESSAGE(status  ${OPENSSL_INCLUDE_DIR})
target_include_directories(tplink_sms PUBLIC ${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${RapidJSON_INCLUDE_DIR})
target_link_libraries(tplink_sms ${CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})
