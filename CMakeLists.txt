cmake_minimum_required(VERSION 3.0)

project(tplinkpp
	VERSION "0.3.1"
	LANGUAGES CXX
)

FIND_PACKAGE(RapidJSON REQUIRED)
FIND_PACKAGE(OpenSSL 3.0 REQUIRED) # for MD5 hash
FIND_PACKAGE(CURL REQUIRED)

FIND_PACKAGE(Doxygen)
IF (DOXYGEN_FOUND)
	option(BUILD_DOC "Build documentation" OFF)
ENDIF()

option(NEW_FIRMWARE "Use ON for latest firmware (M7350(EU)_V5_201019), OFF for others." OFF)

INCLUDE(GNUInstallDirs)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NEW_FIRMWARE)
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNEW_FIRMWARE=1")
else()
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DNEW_FIRMWARE=0")
endif()
set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG")

add_library(tplinkpp SHARED tplink_m7350.cxx)
target_include_directories(tplinkpp PUBLIC ${CURL_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIR} ${RapidJSON_INCLUDE_DIR})
target_link_libraries(tplinkpp ${CURL_LIBRARIES} ${OPENSSL_CRYPTO_LIBRARIES})
set_target_properties(tplinkpp PROPERTIES VERSION ${PROJECT_VERSION})
install(TARGETS tplinkpp LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/tplinkpp)

add_executable(send_sms send_sms.cxx)
target_link_libraries(send_sms tplinkpp)
install(TARGETS send_sms DESTINATION bin)

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
