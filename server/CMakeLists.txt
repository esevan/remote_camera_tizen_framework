cmake_minimum_required(VERSION 2.6)
project(rkf_server CXX)

#add_definitions(-Wall -g -DUSE_FILE_DEBUG)
#add_definitions(-Wall -g -D_DEBUG)
##add_definitions(-Wall -g -pg)

PKG_SEARCH_MODULE(bluetooth REQUIRED capi-network-bluetooth)
PKG_SEARCH_MODULE(system-info REQUIRED capi-system-info)

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${bluetooth_INCLUDE_DIRS}
	${system-info_INCLUDE_DIRS}
	)

ADD_EXECUTABLE(${PROJECT_NAME}
	src/main.cpp
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${rpkgs_LDFLAGS})
SET(CMAKE_CXX_FLAGS "-ldl -lpthread ${CMAKE_CXX_FLAGS}")


INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
