cmake_minimum_required(VERSION 3.10)
project(komozoievm)

include(FetchContent)

FetchContent_Declare(
        excessive
        GIT_REPOSITORY https://gitea.com/komozoi/excessive.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_PROGRESS ON
        SYSTEM
)

FetchContent_MakeAvailable(excessive)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D LIBUS_NO_SSL -Wall -Wextra -Werror -Wno-format -Wno-unused-parameter -Wno-unused-result -O1")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fexec-charset=UTF-8 -fextended-identifiers -finput-charset=UTF-8") # -mavx512f -mavx512dq
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D LIBUS_NO_SSL -Wno-format -Wno-unused-parameter -Wno-unused-result -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

include_directories(src)
include_directories(src/include)

file(GLOB SOURCES
        "src/crypto/*.cpp"
        "src/ethereum/*.cpp"
        "src/interface/*.cpp"
        "src/protocols/*.cpp"
        "src/services/*.cpp"
        "src/storage/*.cpp"
        "src/util/*.cpp"
)

add_library(komozoievm ${SOURCES})
target_link_libraries(komozoievm PRIVATE excessive)

find_package(GTest)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    file(GLOB_RECURSE TEST_SOURCES
            "src/algorithms/tests/*.cpp"
            "src/crypto/tests/*.cpp"
            "src/ethereum/tests/*.cpp"
            "src/events/tests/*.cpp"
            "src/interface/tests/*.cpp"
            "src/protocols/tests/*.cpp"
            "src/services/tests/*.cpp"
            "src/storage/tests/*.cpp"
            "src/util/tests/*.cpp"
    )
    add_executable(tests src/testmain.cpp ${TEST_SOURCES})
    target_link_libraries(tests ${GTEST_LIBRARIES} pthread rt komozoievm excessive)
endif()
