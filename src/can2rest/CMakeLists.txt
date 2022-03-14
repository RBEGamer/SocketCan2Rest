cmake_minimum_required(VERSION 3.16)
project(can2rest)
set(PROJECT_NAME can2rest)
# project version
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
add_compile_definitions(VERSION="0.1.0")




# enable c++
# enable c++
enable_language(C CXX)

set (CMAKE_VERBOSE_MAKEFILE 1)
set(CMAKE_CXX_STANDARD 17)
add_definitions(-std=c++17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

# ENABLE REDIS SUPPORT


#if (ENABLE_REDIS_SUPPORT)
    add_compile_definitions(ENABLE_REDIS_SUPPORT)
    set(ENABLE_REDIS_SUPPORT)
#endif ()

find_package (Threads)



set(SOURCE_FILES
        SHARED/cpp-httplib/httplib.h
        SHARED/hash-library/sha256.cpp
        SHARED/hash-library/sha256.h
        SHARED/json11/json11.hpp SHARED/json11/json11.cpp
        SHARED/socketcan-cpp/src/socketcan_cpp.cpp
        SHARED/loguru/loguru.cpp
        SHARED/inih/INIReader.h
        SHARED/magic_enum/include/magic_enum.hpp

        can_message.h can_message.cpp
        can_frame_to_json.h can_frame_to_json.cpp
        can_hashtable.h can_hashtable.cpp
        can_id_filter.cpp can_id_filter.h
        config_parser.cpp config_parser.h
        rest_api.h rest_api.cpp
        frame_send_queue.cpp frame_send_queue.h
        redis_interface.cpp redis_interface.h
        can2rest.cpp
        )

file(GLOB SOURCES *.h *.hpp *.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${SOURCES})
########################## DEFINES ###############################

# MAKE IT COMPILE ON MAC
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
    set(__APPLE__)
    add_compile_definitions(__APPLE__)
else()
    add_compile_definitions(HAVE_SOCKETCAN_HEADERS)
endif()





# MAC COMPILE ON MAC OSX
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
else()
    TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${Boost_LIBRARIES} rt)
endif()



target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})


#if(ENABLE_REDIS_SUPPORT)
# <------------ add hiredis dependency --------------->
find_path(HIREDIS_HEADER hiredis)
target_include_directories(${PROJECT_NAME} PUBLIC ${HIREDIS_HEADER})

find_library(HIREDIS_LIB hiredis)
target_link_libraries(${PROJECT_NAME} ${HIREDIS_LIB})

# <------------ add redis-plus-plus dependency -------------->
# NOTE: this should be *sw* NOT *redis++*
find_path(REDIS_PLUS_PLUS_HEADER sw)
target_include_directories(${PROJECT_NAME} PUBLIC ${REDIS_PLUS_PLUS_HEADER})

find_library(REDIS_PLUS_PLUS_LIB redis++)
target_link_libraries(${PROJECT_NAME} ${REDIS_PLUS_PLUS_LIB})
#endif()



set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "can2rest")
set(dir ${CMAKE_CURRENT_SOURCE_DIR})
set(EXECUTABLE_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)
set(LIBRARY_OUTPUT_PATH ${dir} CACHE PATH "Build directory" FORCE)