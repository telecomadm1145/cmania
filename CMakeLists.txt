cmake_minimum_required(VERSION 3.20)
project(cmania.linux VERSION 0.1)
#如果是windows系统
if (WIN32)
message(FATAL_ERROR "Use CMake in linux only!")
endif()
aux_source_directory("${PROJECT_SOURCE_DIR}/cmania/" SOURCE_FILES_LIST)
#排除windows文件
list(REMOVE_ITEM SOURCE_FILES_LIST "${PROJECT_SOURCE_DIR}/cmania//Win32ConsoleComponent.cpp")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED 20)
SET(CMAKE_BUILD_TYPE "Debug")
include_directories("${PROJECT_SOURCE_DIR}/third_party/bass/bass24-linux")
add_library(bass SHARED IMPORTED)
#TODO: 支持非x86_64的架构的动态库
set_target_properties(bass PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/bass/bass24-linux/libs/x86_64/libbass.so)
add_custom_target(GIT_INFO
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/cmania"
                COMMAND bash ./GitInfo.sh
                
                BYPRODUCTS "${PROJECT_SOURCE_DIR}/cmania/git_info.h"
)
find_package(Libevent REQUIRED)
add_executable(${PROJECT_NAME} ${SOURCE_FILES_LIST})
add_dependencies(${PROJECT_NAME} GIT_INFO)
target_link_libraries(${PROJECT_NAME} bass)
target_link_libraries(${PROJECT_NAME} libevent::core)
#Capabilities
target_link_libraries(${PROJECT_NAME} cap)

