#######################################################
### Dependencies                                    ###
#######################################################
# Threads
find_package(Threads)

# Ranges library
CPMAddPackage(NAME range-v3 URL https://github.com/ericniebler/range-v3/archive/0.11.0.zip VERSION 0.11.0 DOWNLOAD_ONLY TRUE)
add_library(range-v3 INTERFACE IMPORTED)
target_include_directories(range-v3 INTERFACE "${range-v3_SOURCE_DIR}/include")

include(FetchContent)

# Howard Hinnant's date library
find_package(CURL QUIET)
if (NOT CURL_FOUND)
    set(USE_SYSTEM_TZ_DB ON CACHE BOOL "Build tz library" FORCE)
else ()
    set(USE_SYSTEM_TZ_DB OFF CACHE BOOL "Build tz library" FORCE)
endif ()
message("USE_SYSTEM_TZ_DB=${USE_SYSTEM_TZ_DB}")
set(BUILD_TZ_LIB ON CACHE BOOL "Build tz library" FORCE)
FetchContent_Declare(date
        GIT_REPOSITORY https://github.com/HowardHinnant/date.git
        GIT_TAG v3.0.0
        )

FetchContent_MakeAvailable(date)


# Library: C++ Requests: Curl for People
FetchContent_Declare(cpr
        GIT_REPOSITORY https://github.com/whoshuu/cpr.git
        GIT_TAG 41fbaca90160950f1397e0ffc6b58bd81063f131 # the commit hash for 1.5.2
        )
if (NOT cpr_POPULATED)
    IF (WIN32)
        set(CMAKE_USE_OPENSSL OFF)
        set(USE_OPENSSL OFF)
        set(BUILD_CPR_TESTS OFF)
        set(CMAKE_USE_WINSSL ON)
        set(USE_WINSSL ON)
    ENDIF ()
endif ()
FetchContent_MakeAvailable(cpr)

# Library nlohmann/json
FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.7.3
        )

FetchContent_GetProperties(json)
if (NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

#Library pareto
FetchContent_Declare(pareto
        GIT_REPOSITORY https://github.com/alandefreitas/pareto
        GIT_TAG v1.2.0
        )

FetchContent_GetProperties(pareto)
if (NOT pareto_POPULATED)
    FetchContent_Populate(pareto)
    add_subdirectory(${pareto_SOURCE_DIR} ${pareto_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

#Library matplot
FetchContent_Declare(matplotplusplus
        GIT_REPOSITORY https://github.com/alandefreitas/matplotplusplus
        GIT_TAG v1.1.0
        )

FetchContent_GetProperties(matplotplusplus)
if (NOT matplotplusplus_POPULATED)
    FetchContent_Populate(matplotplusplus)
    add_subdirectory(${matplotplusplus_SOURCE_DIR} ${matplotplusplus_BINARY_DIR} EXCLUDE_FROM_ALL)
endif ()

#######################################################
### Library                                         ###
#######################################################
add_library(portfolio
        portfolio/data_feed/data_feed_result.cpp
        portfolio/data_feed/data_feed_result.h
        portfolio/data_feed/data_feed.cpp
        portfolio/data_feed/data_feed.h
        portfolio/data_feed/mock_data_feed.cpp
        portfolio/data_feed/mock_data_feed.h
        portfolio/data_feed/alphavantage_data_feed.cpp
        portfolio/data_feed/alphavantage_data_feed.h
        portfolio/market_data.cpp
        portfolio/market_data.h
        portfolio/portfolio.cpp
        portfolio/portfolio.h
        portfolio/common/algorithm.h
        portfolio/common/algorithm.cpp
        portfolio/core/ohlc_prices.h
        portfolio/core/ohlc_prices.cpp
        portfolio/portfolio_mad.cpp
        portfolio/portfolio_mad.h
        portfolio/evolutionary_algorithm.h
        portfolio/evolutionary_algorithm.cpp
        portfolio/random_search.h
        portfolio/random_search.cpp)
target_include_directories(portfolio
        PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)


target_link_libraries(portfolio PUBLIC Threads::Threads range-v3 date::date nlohmann_json::nlohmann_json cpr::cpr
        pareto Matplot++::matplot)
target_pedantic_options(portfolio)
target_exception_options(portfolio)