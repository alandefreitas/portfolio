#######################################################
### Google benchmark                                ###
#######################################################
CPMAddPackage(
        NAME benchmark
        GITHUB_REPOSITORY google/benchmark
        VERSION 1.5.1
        OPTIONS
        "BENCHMARK_ENABLE_TESTING Off"
        "LLVMAR_EXECUTABLE ON"
        "LLVMNM_EXECUTABLE ON"
        "LLVMRANLIB_EXECUTABLE ON"
        "GCC_AR ON"
        "GCC_RANLIB ON"
        "BENCHMARK_ENABLE_INSTALL OFF"
)

if (benchmark_ADDED)
    # patch google benchmark target
    set_target_properties(benchmark PROPERTIES CXX_STANDARD 17)
    target_compile_definitions(benchmark PUBLIC -DNDEBUG)
endif()

#######################################################
### Portfolio benchmarks                            ###
#######################################################
# run with "--benchmark_repetitions=30 --benchmark_display_aggregates_only=true --benchmark_out=data_feed_benchmark.csv --benchmark_out_format=csv"
add_executable(data_feed_benchmark data_feed_benchmark.cpp)
add_executable(random_portfolio_benchmark random_portfolio_benchmark.cpp)
target_link_libraries(data_feed_benchmark PUBLIC portfolio benchmark)
target_link_libraries(random_portfolio_benchmark PUBLIC portfolio benchmark)
if (BUILD_LONG_TESTS)
    target_compile_definitions(data_feed_benchmark PUBLIC BUILD_LONG_TESTS)
    target_compile_definitions(random_portfolio_benchmark PUBLIC BUILD_LONG_TESTS)
endif()
if (MSVC)
    # Allow MSVC to compile such a large code
    target_compile_options(data_feed_benchmark PRIVATE /bigobj)
    target_compile_options(random_portfolio_benchmark PRIVATE /bigobj)
    # MSVC requires this flag if the code uses C++ exception handling
    target_compile_options(data_feed_benchmark PRIVATE /EHsc)
    target_compile_options(random_portfolio_benchmark PRIVATE /EHsc)
endif()