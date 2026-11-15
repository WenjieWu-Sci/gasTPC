cmake_minimum_required(VERSION 3.16)
project(gasTPC CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================
# Geant4 (required)  –  needs ui_all vis_all gdml components
# ============================================================
find_package(Geant4 REQUIRED ui_all vis_all gdml)
include(${Geant4_USE_FILE})

message(STATUS "Geant4 version : ${Geant4_VERSION}")
message(STATUS "Geant4 includes: ${Geant4_INCLUDE_DIRS}")

# ============================================================
# ROOT (optional)  –  enables TTree/TFile output
# ============================================================
find_package(ROOT QUIET CONFIG)
if(ROOT_FOUND)
    message(STATUS "ROOT version   : ${ROOT_VERSION}")
    include(${ROOT_USE_FILE})
    add_compile_definitions(HAS_ROOT)
    set(ROOT_LINK_LIBS ROOT::Core ROOT::RIO ROOT::Tree)
else()
    message(STATUS "ROOT not found – text-only output will be used")
    set(ROOT_LINK_LIBS "")
endif()

# ============================================================
# Compiler flags
# ============================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================
# Sources
# ============================================================
include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cc)

add_executable(gasTPC ${SOURCES})

target_link_libraries(gasTPC
    PRIVATE
        ${Geant4_LIBRARIES}
        ${ROOT_LINK_LIBS}
)

# RHEL-family Qt6 builds tag libstdc++ symbols with a "Qt_6" version node that
# no library provides; Geant4's Qt-enabled shared libs then fail strict
# versioned-symbol resolution at link time. The dynamic loader falls back to
# the unversioned libstdc++ symbol at runtime, so allow it here.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(gasTPC PRIVATE -Wl,--allow-shlib-undefined)
endif()

# ============================================================
# Copy runtime assets to the build directory
# ============================================================
file(COPY ${PROJECT_SOURCE_DIR}/macros DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/config DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/gdml   DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# ============================================================
# Install rules
# ============================================================
install(TARGETS gasTPC DESTINATION bin)
install(DIRECTORY macros config gdml DESTINATION share/gasTPC)
