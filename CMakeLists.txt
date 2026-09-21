cmake_minimum_required(VERSION 3.22)
project(AreteUtilities VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# Arete Utilities — one coherent ecosystem of desktop utilities
# ============================================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(ARETE_BUILD_TESTS "Build tests for all components" OFF)

# ----------------------------------------------------------------------------
# Shared core library
# ----------------------------------------------------------------------------
add_subdirectory(AreteCore)

# Let every app locate the shared build tree (used by the update button).
target_compile_definitions(AreteCore PUBLIC ARETE_SOURCE_ROOT="${CMAKE_SOURCE_DIR}")

# ----------------------------------------------------------------------------
# Applications
# ----------------------------------------------------------------------------
add_subdirectory(Logos)
add_subdirectory(Phonio)
add_subdirectory(Visio)
add_subdirectory(Codex)
add_subdirectory(Chronos)
add_subdirectory(Arete)

# ----------------------------------------------------------------------------
# Install helper
# ----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS AreteCore
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
