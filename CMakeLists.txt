project(libjaguar)

set(CMAKE_CXX_STANDARD 17)

add_library(libjaguar SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/src/libjaguar.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/canutil.c
)

target_compile_definitions(libjaguar PUBLIC
	-DCANDRIVER_WIN32=1
)

set(LIBJAGUAR_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(libjaguar
    PUBLIC 
    ${LIBJAGUAR_INCLUDES}
)

add_executable(profiler
	${CMAKE_CURRENT_SOURCE_DIR}/src/profile.c
)

target_link_libraries(profiler libjaguar)

add_executable(testjag
	${CMAKE_CURRENT_SOURCE_DIR}/src/test.c
)

target_link_libraries(testjag libjaguar)