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
    VERSION 0.3.2
    LANGUAGES "C"
    DESCRIPTION "A collection of data structures and algorithms"
)


set(CMAKE_C_COMPILER gcc)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(SSL_DIR /usr/local/ssl)
set(OPENSSL_ROOT_DIR ${SSL_DIR}/lib64)
include(FindOpenSSL)
find_package(OpenSSL)
if(OpenSSL_FOUND)
    include_directories(${SSL_DIR}/include)
else()
    message("OpenSSL not found")
endif()


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

add_library(threadpool_attributes OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool_attributes.c)
target_link_libraries(threadpool_attributes buildingblocks )
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool_attributes.c)

add_library(threadpool OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool.c)
target_link_libraries(threadpool buildingblocks threadpool_attributes queue_concurrent Threads::Threads)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/threadpool.c)

add_library(weighted_graph OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/weighted_graph.c)
target_link_libraries(weighted_graph buildingblocks queue_p queue linked_list hash_table)
set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/weighted_graph.c)

if(OpenSSL_FOUND)
    add_compile_definitions(SSL_AVAILABLE=1)
    add_library(serialization OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization_ssl.c)
    target_link_libraries(serialization buildingblocks OpenSSL::SSL)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization_ssl.c)
else()
    add_compile_definitions(SSL_AVAILABLE=0)
    add_library(serialization OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization.c)
    target_link_libraries(serialization buildingblocks)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/serialization.c)
endif()

add_library(networking_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/src/networking_server.c)
target_link_libraries(networking_server buildingblocks linked_list array_list hash_table threadpool)
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

    add_executable(test_threadpool ${CMAKE_CURRENT_SOURCE_DIR}/tests/threadpool_tests.c)
    target_link_libraries(test_threadpool buildingblocks threadpool_attributes threadpool queue_concurrent queue linked_list cunit)
    add_test(NAME TestThreadPool COMMAND test_threadpool)

    add_library(hero OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/single_thread/hero.c)
    add_library(hero_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/single_thread/hero_server.c)
    add_executable(test_net_adventure ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/single_thread/adventure.c)
    add_executable(test_net_hero ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/single_thread/hero_client.c)

    add_library(threaded_utils OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/multi_thread/threaded_utils.c)
    add_library(threaded_utils_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/multi_thread/threaded_utils_server.c)
    add_executable(threaded_server ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/multi_thread/server.c)
    add_executable(client ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/multi_thread/client.c)

    if(OpenSSL_FOUND)
        target_link_libraries(test_net_adventure buildingblocks serialization networking_server hero hero_server queue_concurrent queue linked_list array_list hash_table threadpool_attributes threadpool OpenSSL::SSL)
        target_link_libraries(test_net_hero buildingblocks serialization hero cunit OpenSSL::SSL)

        target_link_libraries(threaded_server buildingblocks serialization networking_server threaded_utils threaded_utils_server queue_concurrent queue linked_list array_list hash_table threadpool_attributes threadpool Threads::Threads OpenSSL::SSL)
        target_link_libraries(client buildingblocks serialization threaded_utils cunit Threads::Threads OpenSSL::SSL)

        add_library(secure_utils OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/secure/secure_utils.c)
        add_library(secure_utils_server OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/secure/secure_utils_server.c)
        add_executable(secure_server ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/secure/server.c)
        add_executable(secure_client ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/secure/client.c)
        target_link_libraries(secure_server buildingblocks serialization networking_server secure_utils secure_utils_server queue_concurrent queue linked_list array_list hash_table threadpool_attributes threadpool OpenSSL::SSL)
        target_link_libraries(secure_client buildingblocks serialization secure_utils cunit OpenSSL::SSL)
        add_test(NAME TestSSL COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/test_networking.sh ${CMAKE_CURRENT_BINARY_DIR}/secure_server ${CMAKE_CURRENT_BINARY_DIR}/secure_client)
    else()
        target_link_libraries(test_net_adventure buildingblocks serialization networking_server hero hero_server queue_concurrent queue linked_list array_list hash_table threadpool_attributes threadpool)
        target_link_libraries(test_net_hero buildingblocks serialization hero cunit)

        target_link_libraries(threaded_server buildingblocks serialization networking_server threaded_utils threaded_utils_server queue_concurrent queue linked_list array_list hash_table threadpool_attributes threadpool Threads::Threads)
        target_link_libraries(client buildingblocks serialization threaded_utils cunit Threads::Threads)
    endif()
    add_test(NAME TestNetworking COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/test_networking.sh ${CMAKE_CURRENT_BINARY_DIR}/test_net_adventure ${CMAKE_CURRENT_BINARY_DIR}/test_net_hero)
    add_test(NAME TestThreadedNetworking COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/networking/test_networking.sh ${CMAKE_CURRENT_BINARY_DIR}/threaded_server ${CMAKE_CURRENT_BINARY_DIR}/client)
endif()
