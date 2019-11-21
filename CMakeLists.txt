cmake_minimum_required(VERSION 3.0)

project(tplink_send_sms)

FIND_PACKAGE(RapidJSON REQUIRED)
FIND_PACKAGE(OpenSSL REQUIRED) # for MD5 hash
FIND_PACKAGE(CURL REQUIRED)

FIND_PACKAGE(Doxygen)
IF (DOXYGEN_FOUND)
	option(BUILD_DOC "Build documentation" ON)
ENDIF()

set (CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -std=c++11)

add_executable(send_sms tplink_m7350.cxx send_sms.cxx)

target_include_directories(send_sms PUBLIC ${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${RapidJSON_INCLUDE_DIR})
target_link_libraries(send_sms ${CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})

# Documentation
IF (DOXYGEN_FOUND AND BUILD_DOC)
  set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in)
  set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
	configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
	add_custom_target( doc_doxygen ALL
		COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating API documentation with Doxygen"
		VERBATIM )
		
		install( DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doc/html DESTINATION share/doc/tplink_m7350 )
ENDIF()
