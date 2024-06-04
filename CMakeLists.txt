cmake_minimum_required(VERSION 3.25)
project(libjaguar)

set(CMAKE_CXX_STANDARD 17)

add_library(libjaguar SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libjaguar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canutil.c
)

target_compile_definitions(libjaguar PUBLIC
	-DCANDRIVER_SERIAL=1
)

set(LIBJAGUAR_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(libjaguar
    PUBLIC 
    ${LIBJAGUAR_INCLUDES}
)
set_target_properties(libjaguar PROPERTIES PUBLIC_HEADER "include/canutil.h;include/can.h;include/libjaguar.h")
INSTALL(TARGETS libjaguar)

add_executable(profiler
	${CMAKE_CURRENT_SOURCE_DIR}/src/profile.c
)

target_link_libraries(profiler libjaguar)

