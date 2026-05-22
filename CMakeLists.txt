cmake_minimum_required(VERSION 3.31)
project(spcf)

set(Rust_TOOLCHAIN nightly)
include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6 # Optionally specify a commit hash, version tag or branch here
)
FetchContent_MakeAvailable(Corrosion)

corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

find_package(OpenSSL REQUIRED)

set(MILSKO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/milsko)
add_subdirectory(${MILSKO_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} spcf_rust sqlite3 Mw ${OPENSSL_LIBRARIES})
if(UNIX)
  target_link_libraries(${PROJECT_NAME} blkid)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${MILSKO_DIR}/include)
