cmake_minimum_required(VERSION 3.22) # For Ubuntu 22.04 compatibility
project(akprag_fss LANGUAGES C CXX)

include(CTest)

set(FSS_kLambda 16 CACHE STRING "Custom kLambda")

find_package(OpenMP REQUIRED)
find_package(OpenSSL REQUIRED)

include(FetchContent)
if(BUILD_TESTING)
    FetchContent_Declare(
        GTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG e90fe2485641bab0d6af4500192dc503384950d1
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        # For Windows: Prevent overriding the parent project's compiler/linker settings
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(GTest)
    include(GoogleTest)
endif()

add_compile_options(-O3) # It does improve performance

add_library(dcf STATIC src/dcf/dcf.c)
target_compile_definitions(dcf PUBLIC kLambda=${FSS_kLambda})
target_include_directories(dcf PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
if(OpenMP_FOUND)
    target_link_libraries(dcf PUBLIC OpenMP::OpenMP_C)
endif()

add_executable(dcf_benchmark src/dcf.c src/dcf/group/u64.c src/dcf/prg/aes128_mmo.c)
target_compile_definitions(dcf_benchmark PRIVATE kLambda=${FSS_kLambda} kBlocks=4)
target_link_libraries(dcf_benchmark PRIVATE dcf OpenSSL::Crypto OpenMP::OpenMP_C)

add_executable(cmp_benchmark src/cmp.c src/dcf/group/u64.c src/dcf/prg/aes128_mmo.c)
target_compile_definitions(cmp_benchmark PRIVATE kLambda=${FSS_kLambda} kBlocks=4)
target_link_libraries(cmp_benchmark PRIVATE dcf OpenSSL::Crypto OpenMP::OpenMP_C)

add_executable(dotprod_benchmark src/dotprod.c)
target_link_libraries(dotprod_benchmark PRIVATE OpenMP::OpenMP_C)

add_executable(retrieval src/retrieval.c src/dcf/group/u64.c src/dcf/prg/aes128_mmo.c)
target_compile_definitions(retrieval PRIVATE kLambda=${FSS_kLambda} kBlocks=4)
target_link_libraries(retrieval PRIVATE dcf OpenSSL::Crypto OpenMP::OpenMP_C)

if(BUILD_TESTING)
    add_executable(
        dcf_u64_test src/dcf/dcf_test.cc
        src/dcf/group/u64.c
        src/dcf/prg/aes128_mmo.c
    )
    target_compile_definitions(dcf_u64_test PRIVATE -DkBlocks=4)
    target_link_libraries(dcf_u64_test GTest::gtest_main dcf OpenSSL::Crypto)
    gtest_discover_tests(dcf_u64_test)
endif()
