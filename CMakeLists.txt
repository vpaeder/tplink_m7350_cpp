cmake_minimum_required(VERSION 3.0)

project(tplink_sms)

FIND_PACKAGE(RapidJSON REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED) # for MD5 hash
FIND_PACKAGE(CURL REQUIRED)

set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)

add_executable(tplink_sms tplink_sms.cxx)

target_include_directories(tplink_sms PUBLIC ${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${RapidJSON_INCLUDE_DIR})
target_link_libraries(tplink_sms ${CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})
