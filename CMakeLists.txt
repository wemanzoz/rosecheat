cmake_minimum_required(VERSION 3.16.0)
project(rosecheat)

set(CMAKE_C_FLAGS_RELEASE "-O3 -fvisibility=hidden -flto -fno-exceptions -DNDEBUG -Wfatal-errors")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fno-rtti")

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} --no-gnu-unique")
endif()

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/rosecheat/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rosecheat/imgui/imgui_impl_dx9.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rosecheat/imgui/imgui_impl_win32.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rosecheat/Hooks/MinHook.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rosecheat/pch.cpp") 

find_package(Freetype REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(rosecheat SHARED ${SOURCES})

target_include_directories(rosecheat PRIVATE ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_precompile_headers(rosecheat PRIVATE "${CMAKE_SOURCE_DIR}/rosecheat/pch.h")
target_link_libraries(rosecheat PRIVATE SDL2 GL ${CMAKE_DL_LIBS} Freetype::Freetype)
target_link_options(rosecheat PRIVATE LINKER:--no-undefined)
