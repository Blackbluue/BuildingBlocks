cmake_minimum_required(VERSION 3.16)

enable_testing()
set(CTEST_RERUN_FAILED ON)
set(CTEST_OUTPUT_ON_FAILURE ON)

project(
    BuildingBlocks
    VERSION 0.0.0
    LANGUAGES "C"
    DESCRIPTION "A collection of data structures and algorithms"
)

include_directories(${buildingblocks_SOURCE_DIR}/include)

find_program(CLANG_TIDY_PROG clang-tidy)
if(CLANG_TIDY_PROG)
    set(CMAKE_C_CLANG_TIDY
    ${CLANG_TIDY_PROG};
    "--checks=* -llvm-include-order, -cppcoreguidelines-*, -readability-magic-numbers, -clang-analyzer-security.insecureAPI.DeprecatedOrUnsafeBufferHandling, -hiccp-no-assembler")
    set(CMAKE_C_FLAGS "-g")
else()
    set(CMAKE_C_COMPILER gcc)
    set(CMAKE_C_FLAGS "-g -Wall -pedantic")
endif()

if(DEBUG EQUAL "1")
    message("DEBUGGING VERSION")
    set(CMAKE_BUILD_TYPE Debug)
    add_definitions(-DDEBUG)
else()
    message("RELEASE VERSION")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


### DATA STRUCTURES ###


add_library(array_list SHARED ${datastructures1_SOURCE_DIR}/src/array_list.c)
add_executable(test_array_list ${datastructures1_SOURCE_DIR}/tests/array_list_tests.c)
target_link_libraries(test_array_list array_list cunit)
# INSTALL(TARGETS array_list DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(linked_list SHARED ${datastructures1_SOURCE_DIR}/src/linked_list.c)
# add_executable(test_list ${datastructures1_SOURCE_DIR}/tests/linked_list_tests.c)
# target_link_libraries(test_list linked_list cunit)
# # INSTALL(TARGETS linked_list test_list DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(hash_table SHARED ${datastructures1_SOURCE_DIR}/src/hash_table.c)
# add_executable(test_table ${datastructures1_SOURCE_DIR}/tests/hash_table_tests.c)
# target_link_libraries(test_table hash_table linked_list cunit)
# # INSTALL(TARGETS test_table hash_table DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(stack SHARED ${datastructures1_SOURCE_DIR}/src/stack.c)
# add_executable(test_stack ${datastructures1_SOURCE_DIR}/tests/stack_tests.c)
# target_link_libraries(test_stack stack cunit)
# # INSTALL(TARGETS test_stack stack DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(queue SHARED ${datastructures1_SOURCE_DIR}/src/queue.c)
# add_executable(test_queue ${datastructures1_SOURCE_DIR}/tests/queue_tests.c)
# target_link_libraries(test_queue queue linked_list cunit)
# # INSTALL(TARGETS test_queue queue DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(queue_p SHARED ${datastructures1_SOURCE_DIR}/src/queue_p.c)
# add_executable(test_queue_p ${datastructures1_SOURCE_DIR}/tests/queue_p_tests.c)
# target_link_libraries(test_queue_p queue_p queue linked_list cunit)
# # INSTALL(TARGETS test_queue_p queue_p DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(avl_tree SHARED ${datastructures1_SOURCE_DIR}/src/avl_tree.c)
# add_executable(test_tree ${datastructures1_SOURCE_DIR}/tests/avl_tree_tests.c)
# target_link_libraries(test_tree avl_tree queue linked_list cunit)
# # INSTALL(TARGETS test_avl_tree avl_tree DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(queue_concurrent SHARED ${datastructures1_SOURCE_DIR}/src/queue_concurrent.c)
# target_link_libraries(queue_concurrent queue linked_list cunit)
# # INSTALL(TARGETS test_queue_concurrent queue_concurrent DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(threadpool SHARED ${datastructures1_SOURCE_DIR}/src/threadpool.c)
# add_executable(test_threadpool ${datastructures1_SOURCE_DIR}/tests/threadpool_tests.c)
# target_link_libraries(test_threadpool threadpool queue_concurrent queue linked_list  Threads::Threads cunit)
# # INSTALL(TARGETS test_threadpool threadpool DESTINATION ${datastructures1_SOURCE_DIR}/build)

# add_library(weighted_graph SHARED ${datastructures1_SOURCE_DIR}/src/weighted_graph.c)
# target_link_libraries(weighted_graph queue_p queue linked_list hash_table)
# # INSTALL(TARGETS test_weighted_graph weighted_graph DESTINATION ${datastructures1_SOURCE_DIR}/build)
