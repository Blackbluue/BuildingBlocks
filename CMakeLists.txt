cmake_minimum_required(VERSION 3.16)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("DEBUGGING VERSION")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message("RELEASE VERSION")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    message("RELEASE WITH DEBUGGING INFO")
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    message("MINIMAL SIZE RELEASE")
endif()

project(
    BuildingBlocks
    VERSION 0.1.0
    LANGUAGES "C"
    DESCRIPTION "A collection of data structures and algorithms"
)


set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-Wall -pedantic")
# set(THREADS_PREFER_PTHREAD_FLAG ON)
# find_package(Threads REQUIRED)


include_directories(${BuildingBlocks_SOURCE_DIR}/include)
add_library(${PROJECT_NAME} SHARED ${BuildingBlocks_SOURCE_DIR}/src/buildingblocks.c)


### DATA STRUCTURES ###


add_library(array_list OBJECT ${BuildingBlocks_SOURCE_DIR}/src/array_list.c)
target_link_libraries(${PROJECT_NAME} array_list)

add_library(linked_list OBJECT ${BuildingBlocks_SOURCE_DIR}/src/linked_list.c)
target_link_libraries(${PROJECT_NAME} linked_list)

add_library(stack OBJECT ${BuildingBlocks_SOURCE_DIR}/src/stack.c)
target_link_libraries(${PROJECT_NAME} stack)

add_library(hash_table OBJECT ${BuildingBlocks_SOURCE_DIR}/src/hash_table.c)
target_link_libraries(hash_table linked_list)
target_link_libraries(${PROJECT_NAME} hash_table)

add_library(queue OBJECT ${BuildingBlocks_SOURCE_DIR}/src/queue.c)
target_link_libraries(queue linked_list)
target_link_libraries(${PROJECT_NAME} queue)

# add_library(queue_p OBJECT ${BuildingBlocks_SOURCE_DIR}/src/queue_p.c)
# target_link_libraries(queue_p queue)
# target_link_libraries(${PROJECT_NAME} queue_p)

# add_library(avl_tree OBJECT ${BuildingBlocks_SOURCE_DIR}/src/avl_tree.c)
# target_link_libraries(avl_tree queue linked_list)
# target_link_libraries(${PROJECT_NAME} avl_tree)

# add_library(queue_concurrent OBJECT ${BuildingBlocks_SOURCE_DIR}/src/queue_concurrent.c)
# target_link_libraries(queue_concurrent queue linked_list)
# target_link_libraries(${PROJECT_NAME} queue_concurrent)

# add_library(threadpool OBJECT ${BuildingBlocks_SOURCE_DIR}/src/threadpool.c)
# target_link_libraries(threadpool queue_concurrent linked_list Threads::Threads)
# target_link_libraries(${PROJECT_NAME} threadpool)

# add_library(weighted_graph OBJECT ${BuildingBlocks_SOURCE_DIR}/src/weighted_graph.c)
# target_link_libraries(weighted_graph queue_p queue linked_list hash_table)
# target_link_libraries(${PROJECT_NAME} weighted_graph)

# INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BuildingBlocks_SOURCE_DIR}/build)

### TESTS ###


if(BUILD_TESTING)
    enable_testing()
    set(CTEST_RERUN_FAILED ON)
    add_executable(test_array_list ${BuildingBlocks_SOURCE_DIR}/tests/array_list_tests.c)
    target_link_libraries(test_array_list array_list cunit)
    add_test(NAME TestArrayList COMMAND test_array_list)

    add_executable(test_linked_list ${BuildingBlocks_SOURCE_DIR}/tests/linked_list_tests.c)
    target_link_libraries(test_linked_list linked_list cunit)
    add_test(NAME TestLinkedList COMMAND test_linked_list)

    add_executable(test_stack ${BuildingBlocks_SOURCE_DIR}/tests/stack_tests.c)
    target_link_libraries(test_stack stack cunit)
    add_test(NAME TestStack COMMAND test_stack)

    add_executable(test_table ${BuildingBlocks_SOURCE_DIR}/tests/hash_table_tests.c)
    target_link_libraries(test_table hash_table linked_list cunit)
    add_test(NAME TestHashTable COMMAND test_table)

    add_executable(test_queue ${BuildingBlocks_SOURCE_DIR}/tests/queue_tests.c)
    target_link_libraries(test_queue queue linked_list cunit)
    add_test(NAME TestQueue COMMAND test_queue)

    # add_executable(test_queue_p ${BuildingBlocks_SOURCE_DIR}/tests/queue_p_tests.c)
    # target_link_libraries(test_queue_p queue_p cunit)
    # add_test(NAME TestQueueP COMMAND test_queue_p)

    # add_executable(test_tree ${BuildingBlocks_SOURCE_DIR}/tests/avl_tree_tests.c)
    # target_link_libraries(test_tree avl_tree queue linked_list cunit)
    # add_test(NAME TestTree COMMAND test_tree)

    # add_executable(test_threadpool ${BuildingBlocks_SOURCE_DIR}/tests/threadpool_tests.c)
    # target_link_libraries(test_threadpool threadpool cunit)
    # add_test(NAME TestThreadPool COMMAND test_threadpool)
endif()
