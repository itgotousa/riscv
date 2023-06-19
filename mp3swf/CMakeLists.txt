cmake_minimum_required(VERSION 3.26)

project(mp3swf)

add_definitions(-D_UNICODE -DUNICODE)
add_executable(${PROJECT_NAME} WIN32 mp3swf.cpp libmp3dec.c vlc.c bits.c)

# target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/wtl ${CMAKE_SOURCE_DIR} ${LIBSWF_SOURCE_DIR})
# target_link_libraries(${PROJECT_NAME} PUBLIC libswf)

if(MSVC)
    add_compile_options(
        $<$<CONFIG:>:/MT> #---------|
        $<$<CONFIG:Debug>:/MTd> #---|-- Statically link the runtime libraries
        $<$<CONFIG:Release>:/MT> #--|
    )
endif()
