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

file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/rose/*.cpp")
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rose/imgui/imgui_impl_dx9.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rose/imgui/imgui_impl_win32.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rose/Hooks/MinHook.cpp") 
list(REMOVE_ITEM SOURCES "${CMAKE_SOURCE_DIR}/rose/pch.cpp") 

find_package(Freetype REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(rose SHARED ${SOURCES})

target_include_directories(rose PRIVATE ${SDL2_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS})
target_precompile_headers(rose PRIVATE "${CMAKE_SOURCE_DIR}/rose/pch.h")
target_link_libraries(rose PRIVATE SDL2 GL ${CMAKE_DL_LIBS} Freetype::Freetype)
target_link_options(rose PRIVATE LINKER:--no-undefined)