cmake_minimum_required(VERSION 3.31)
project(sortmancer)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENABLE_CUDA "CUDA Support" OFF)

set(Rust_TOOLCHAIN nightly)
include(FetchContent)
FetchContent_Declare(
    Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6 # Optionally specify a commit hash, version tag or branch here
)
FetchContent_MakeAvailable(Corrosion)

if(ENABLE_CUDA)
  corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml FEATURES cuda)
else()
  corrosion_import_crate(MANIFEST_PATH ${CMAKE_CURRENT_SOURCE_DIR}/Cargo.toml)
endif()
FILE(GLOB_RECURSE SOURCES src/*.cpp src/*.c)

find_package(OpenSSL REQUIRED)

set(MILSKO_DIR ${CMAKE_CURRENT_SOURCE_DIR}/submodules/milsko)
set(MW_BUILD_STATIC ON)
add_subdirectory(${MILSKO_DIR})

add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} sortmancer_rust sqlite3 Mw ${OPENSSL_LIBRARIES})
if(ENABLE_CUDA)
  find_package(CUDAToolkit REQUIRED)
  target_link_directories(${PROJECT_NAME} PUBLIC /usr/local/cuda/lib64/)
  target_link_libraries(${PROJECT_NAME} ${CUDAToolkit_LIBRARIES} cuda CUDA::cupti)
endif()
if(UNIX)
  target_link_libraries(${PROJECT_NAME} blkid)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${MILSKO_DIR}/include)
