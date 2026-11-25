cmake_minimum_required(VERSION 3.21)
project(gold_smc VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------
# Compiler-specific flags
# ---------------------------------------------------------------
if(MSVC)
    # MSVC / clang-cl
    add_compile_options(/utf-8 /W3 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
elseif(MINGW)
    # MinGW-w64 g++ on Windows
    add_compile_options(-Wall -Wextra -Wpedantic
                        -finput-charset=UTF-8 -fexec-charset=UTF-8)
    # Fully self-contained exe: no libstdc++-6.dll, libgcc_s_seh-1.dll,
    # libwinpthread-1.dll, or libcurl DLL needed at runtime.
    add_link_options(-static -static-libgcc -static-libstdc++)
else()
    # Linux / macOS / other GCC or Clang
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------
find_package(CURL REQUIRED)

find_package(nlohmann_json 3.11 QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
endif()

# ---------------------------------------------------------------
# Core library
# ---------------------------------------------------------------
add_library(goldsmc_core
    src/config.cpp
    src/utils.cpp
    src/twelvedata.cpp
    src/indicators.cpp
    src/strategy.cpp
)
target_include_directories(goldsmc_core PUBLIC include)
target_link_libraries(goldsmc_core
    PUBLIC  nlohmann_json::nlohmann_json
    PRIVATE CURL::libcurl
)

# ---------------------------------------------------------------
# Executable
# ---------------------------------------------------------------
add_executable(gold_smc src/main.cpp)
target_link_libraries(gold_smc PRIVATE goldsmc_core)

# MSVC + dynamic vcpkg triplet needs runtime DLLs next to the exe.
# MinGW static builds are fully self-contained — skip this on MinGW.
if(MSVC)
    add_custom_command(TARGET gold_smc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:gold_smc>
                $<TARGET_FILE_DIR:gold_smc>
        COMMAND_EXPAND_LISTS)
endif()

# ---------------------------------------------------------------
# Tests
# ---------------------------------------------------------------
enable_testing()
add_subdirectory(tests)