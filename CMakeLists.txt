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
    VERSION 0.1.2
    LANGUAGES "C"
    DESCRIPTION "A collection of data structures and algorithms"
)

# set(THREADS_PREFER_PTHREAD_FLAG ON)
# find_package(Threads REQUIRED)


### INSTALL TARGETS ###

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/src/buildingblocks.c)
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


### DATA STRUCTURES ###


add_library(array_list OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/array_list.c)
target_link_libraries(${PROJECT_NAME} array_list)

add_library(linked_list OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/linked_list.c)
target_link_libraries(${PROJECT_NAME} linked_list)

add_library(stack OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/stack.c)
target_link_libraries(${PROJECT_NAME} stack)

add_library(hash_table OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/hash_table.c)
target_link_libraries(hash_table linked_list)
target_link_libraries(${PROJECT_NAME} hash_table)

add_library(queue OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue.c)
target_link_libraries(queue linked_list)
target_link_libraries(${PROJECT_NAME} queue)

add_library(queue_p OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_p.c)
set_property(TARGET queue_p PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(queue_p queue linked_list)
target_link_libraries(${PROJECT_NAME} queue_p)

add_library(avl_tree OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/avl_tree.c)
target_link_libraries(avl_tree queue linked_list)
target_link_libraries(${PROJECT_NAME} avl_tree)

add_library(queue_concurrent OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/queue_concurrent.c)
set_property(TARGET queue_concurrent PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(queue_concurrent queue linked_list)
target_link_libraries(${PROJECT_NAME} queue_concurrent)

# add_library(threadpool OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool.c)
# target_link_libraries(threadpool queue_concurrent linked_list Threads::Threads)
# target_link_libraries(${PROJECT_NAME} threadpool)

add_library(weighted_graph OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/weighted_graph.c)
target_link_libraries(weighted_graph queue_p queue linked_list hash_table)
target_link_libraries(${PROJECT_NAME} weighted_graph)

add_library(networking_utils OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_utils.c)
target_link_libraries(${PROJECT_NAME} networking_utils)

add_library(networking_client OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_client.c)
target_link_libraries(networking_client networking_utils)
target_link_libraries(${PROJECT_NAME} networking_client)

add_library(networking_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_server.c)
target_link_libraries(networking_server networking_utils)
target_link_libraries(${PROJECT_NAME} networking_server)

### TESTS ###


if(BUILD_TESTING)
    enable_testing()
    set(CTEST_RERUN_FAILED ON)
    add_executable(test_array_list ${CMAKE_CURRENT_SOURCE_DIR}/tests/array_list_tests.c)
    target_link_libraries(test_array_list array_list cunit)
    add_test(NAME TestArrayList COMMAND test_array_list)

    add_executable(test_linked_list ${CMAKE_CURRENT_SOURCE_DIR}/tests/linked_list_tests.c)
    target_link_libraries(test_linked_list linked_list cunit)
    add_test(NAME TestLinkedList COMMAND test_linked_list)

    add_executable(test_stack ${CMAKE_CURRENT_SOURCE_DIR}/tests/stack_tests.c)
    target_link_libraries(test_stack stack cunit)
    add_test(NAME TestStack COMMAND test_stack)

    add_executable(test_table ${CMAKE_CURRENT_SOURCE_DIR}/tests/hash_table_tests.c)
    target_link_libraries(test_table hash_table linked_list cunit)
    add_test(NAME TestHashTable COMMAND test_table)

    add_executable(test_queue ${CMAKE_CURRENT_SOURCE_DIR}/tests/queue_tests.c)
    target_link_libraries(test_queue queue linked_list cunit)
    add_test(NAME TestQueue COMMAND test_queue)

    add_executable(test_queue_p ${CMAKE_CURRENT_SOURCE_DIR}/tests/queue_p_tests.c)
    target_link_libraries(test_queue_p queue_p queue linked_list cunit)
    add_test(NAME TestQueueP COMMAND test_queue_p)

    add_executable(test_tree ${CMAKE_CURRENT_SOURCE_DIR}/tests/avl_tree_tests.c)
    target_link_libraries(test_tree avl_tree queue linked_list cunit)
    add_test(NAME TestTree COMMAND test_tree)

    # add_executable(test_threadpool ${CMAKE_CURRENT_SOURCE_DIR}/tests/threadpool_tests.c)
    # target_link_libraries(test_threadpool threadpool cunit)
    # add_test(NAME TestThreadPool COMMAND test_threadpool)
endif()
