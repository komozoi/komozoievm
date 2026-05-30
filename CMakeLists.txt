cmake_minimum_required(VERSION 3.10)
project(komozoievm)

include(FetchContent)

# Position-independent code is required so the static libraries we build can be
# linked into the shared Python module.  Set it before any subprojects compile.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

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

# Build options are declared up front so they show up in the IDE regardless of
# build configuration.  The Python bindings are built by default; turn them off
# with -DBUILD_PYTHON_BINDINGS=OFF if Python or pybind11 isn't available.
option(BUILD_PYTHON_BINDINGS "Build the komozoievm Python extension module" ON)
option(BUILD_CXX_TESTS "Build the native C++ test executable" ON)


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

# Python bindings.
if(BUILD_PYTHON_BINDINGS)
    # pybind11 is fetched at configure time so source builds work without a
    # pre-installed copy.  Wheel builds via scikit-build-core still satisfy the
    # dependency from the build environment when available.
    find_package(pybind11 CONFIG QUIET)
    if(NOT pybind11_FOUND)
        FetchContent_Declare(
                pybind11
                GIT_REPOSITORY https://github.com/pybind/pybind11.git
                GIT_TAG v2.13.6
                GIT_SHALLOW TRUE
                GIT_PROGRESS ON
                SYSTEM
        )
        FetchContent_MakeAvailable(pybind11)
    endif()

    set(PYTHON_BINDING_SOURCES
            src/python.cpp
    )

    pybind11_add_module(_komozoievm MODULE ${PYTHON_BINDING_SOURCES})
    target_link_libraries(_komozoievm PRIVATE komozoievm excessive)
    target_include_directories(_komozoievm PRIVATE src src/include)
    target_compile_features(_komozoievm PRIVATE cxx_std_17)

    # Install the compiled extension next to the Python package root so that
    # `import komozoievm` works after a wheel install.
    install(TARGETS _komozoievm LIBRARY DESTINATION komozoievm)
    install(FILES python/komozoievm/__init__.py DESTINATION komozoievm)
endif()

if(NOT BUILD_CXX_TESTS)
    return()
endif()

find_package(GTest)
if(GTest_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    file(GLOB TEST_SOURCES
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
    add_executable(evm_tests src/testmain.cpp ${TEST_SOURCES})
    target_link_libraries(evm_tests ${GTEST_LIBRARIES} pthread rt komozoievm excessive)
endif()
