cmake_minimum_required(VERSION 3.16)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("DEBUGGING VERSION")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-fPIC)
    add_compile_definitions(DEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("RELEASE VERSION")
    add_compile_options(-Wall -Wextra)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("RELEASE WITH DEBUGGING INFO")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message("MINIMAL SIZE RELEASE")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

project(
    BuildingBlocks
    VERSION 0.3.0
    LANGUAGES "C"
    DESCRIPTION "A collection of data structures and algorithms"
)


set(CMAKE_C_COMPILER gcc)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SOURCES "")

### DATA STRUCTURES ###


add_library(buildingblocks OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/buildingblocks.c)

add_library(array_list OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/array_list.c)
target_link_libraries(array_list buildingblocks)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/array_list.c)

add_library(linked_list OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/linked_list.c)
target_link_libraries(linked_list buildingblocks)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/linked_list.c)

add_library(stack OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/stack.c)
target_link_libraries(stack buildingblocks)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/stack.c)

add_library(hash_table OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/hash_table.c)
target_link_libraries(hash_table buildingblocks linked_list)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/hash_table.c)

add_library(queue OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue.c)
target_link_libraries(queue buildingblocks linked_list)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/queue.c)

add_library(queue_p OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_p.c)
set_property(TARGET queue_p PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(queue_p buildingblocks queue linked_list)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_p.c)

add_library(avl_tree OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/avl_tree.c)
target_link_libraries(avl_tree buildingblocks queue linked_list)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/avl_tree.c)

add_library(queue_concurrent OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_concurrent.c)
set_property(TARGET queue_concurrent PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(queue_concurrent buildingblocks queue linked_list)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_concurrent.c)

add_library(threadpool OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool.c)
target_link_libraries(threadpool buildingblocks queue_concurrent Threads::Threads)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool.c)

add_library(weighted_graph OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/weighted_graph.c)
target_link_libraries(weighted_graph buildingblocks queue_p queue linked_list hash_table)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/weighted_graph.c)

add_library(serialization OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization.c)
target_link_libraries(serialization buildingblocks)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization.c)

add_library(networking_utils OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_utils.c)
target_link_libraries(networking_utils buildingblocks)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_utils.c)

add_library(networking_client OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_client.c)
target_link_libraries(networking_client buildingblocks networking_utils)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_client.c)

add_library(networking_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_server.c)
target_link_libraries(networking_server buildingblocks networking_utils)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_server.c)

### INSTALL TARGETS ###

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/private_include)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION /lib)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION /include/${PROJECT_NAME})

# uninstall target
if(NOT TARGET uninstall)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)

    add_custom_target(uninstall
        COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
endif()


### TESTS ###


if(BUILD_TESTING)
    enable_testing()
    set(CTEST_RERUN_FAILED ON)
    add_executable(test_array_list ${CMAKE_CURRENT_SOURCE_DIR}/tests/array_list_tests.c)
    target_link_libraries(test_array_list buildingblocks array_list cunit)
    add_test(NAME TestArrayList COMMAND test_array_list)

    add_executable(test_linked_list ${CMAKE_CURRENT_SOURCE_DIR}/tests/linked_list_tests.c)
    target_link_libraries(test_linked_list buildingblocks linked_list cunit)
    add_test(NAME TestLinkedList COMMAND test_linked_list)

    add_executable(test_stack ${CMAKE_CURRENT_SOURCE_DIR}/tests/stack_tests.c)
    target_link_libraries(test_stack buildingblocks stack cunit)
    add_test(NAME TestStack COMMAND test_stack)

    add_executable(test_table ${CMAKE_CURRENT_SOURCE_DIR}/tests/hash_table_tests.c)
    target_link_libraries(test_table buildingblocks hash_table linked_list cunit)
    add_test(NAME TestHashTable COMMAND test_table)

    add_executable(test_queue ${CMAKE_CURRENT_SOURCE_DIR}/tests/queue_tests.c)
    target_link_libraries(test_queue buildingblocks queue linked_list cunit)
    add_test(NAME TestQueue COMMAND test_queue)

    add_executable(test_queue_p ${CMAKE_CURRENT_SOURCE_DIR}/tests/queue_p_tests.c)
    target_link_libraries(test_queue_p buildingblocks queue_p queue linked_list cunit)
    add_test(NAME TestQueueP COMMAND test_queue_p)

    add_executable(test_tree ${CMAKE_CURRENT_SOURCE_DIR}/tests/avl_tree_tests.c)
    target_link_libraries(test_tree buildingblocks avl_tree queue linked_list cunit)
    add_test(NAME TestTree COMMAND test_tree)

    add_library(hero OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/hero.c)
    add_executable(test_net_adventure ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/adventure.c)
    target_link_libraries(test_net_adventure buildingblocks serialization networking_utils networking_server hero)
    add_executable(test_net_hero ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/hero_client.c)
    target_link_libraries(test_net_hero buildingblocks serialization networking_utils networking_client hero cunit)
    add_test(NAME TestNetworking COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/test_networking.sh ${CMAKE_CURRENT_BINARY_DIR}/test_net_adventure ${CMAKE_CURRENT_BINARY_DIR}/test_net_hero)

    add_executable(test_threadpool ${CMAKE_CURRENT_SOURCE_DIR}/tests/threadpool_tests.c)
    target_link_libraries(test_threadpool buildingblocks threadpool queue_concurrent queue linked_list cunit)
    add_test(NAME TestThreadPool COMMAND test_threadpool)
endif()
