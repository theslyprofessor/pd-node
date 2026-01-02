# pd-node - Modern JavaScript & TypeScript for Pure Data
# CMake build configuration
cmake_minimum_required(VERSION 3.13)

set(CMAKE_MACOSX_RPATH Off)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)  # Modern macOS
set(CMAKE_CXX_STANDARD 17)              # C++17 for filesystem, etc.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(pd.build/pd.cmake)

project("pd-node" VERSION 0.1.0)

# Detect target platform
if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
    set(TARGET_CPU x64)
elseif(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ARM64" OR ${CMAKE_SYSTEM_PROCESSOR} STREQUAL "aarch64")
    set(TARGET_CPU arm64)
else()
    set(TARGET_CPU ${CMAKE_SYSTEM_PROCESSOR})
endif()

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
    set(TARGET_OS macos)
else()
    string(TOLOWER ${CMAKE_SYSTEM_NAME} TARGET_OS)
endif()

set(TARGET_TRIPLET ${TARGET_CPU}-${TARGET_OS})
message(STATUS "Building pd-node for: ${TARGET_TRIPLET}")

# Set Pure Data paths
set_pd_sources(${PROJECT_SOURCE_DIR}/pure-data/src/)
set_pd_external_path("${PROJECT_SOURCE_DIR}/binaries/${TARGET_TRIPLET}/")

# Compiler flags
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter -std=c++17)
    add_link_options(-static-libstdc++)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
        add_link_options(-s)  # Strip symbols
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-unused-parameter -std=c++17)
elseif(MSVC)
    add_compile_options(/W4 /std:c++17)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

# Version info
add_compile_definitions(
    PD_NODE_VERSION="${PROJECT_VERSION}"
    PD_NODE_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    PD_NODE_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    PD_NODE_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Source files
set(PD_NODE_SOURCES
    ${PROJECT_SOURCE_DIR}/node/node.cpp
    ${PROJECT_SOURCE_DIR}/node/runtime_detector.cpp
    ${PROJECT_SOURCE_DIR}/node/ipc_bridge.cpp
)

# Create pd-node external
add_pd_external(pd_node_project node ${PD_NODE_SOURCES})

# Copy help files and pd-api to output
configure_file(node/node-help.pd ${PD_OUTPUT_PATH}/node-help.pd COPYONLY)
file(COPY ${PROJECT_SOURCE_DIR}/pd-api DESTINATION ${PD_OUTPUT_PATH})

# Platform-specific linking
if(UNIX AND NOT APPLE)
    target_link_libraries(pd_node_project pthread)
elseif(APPLE)
    # macOS might need CoreFoundation for process management
    # target_link_libraries(pd_node_project "-framework CoreFoundation")
elseif(WIN32)
    target_link_libraries(pd_node_project ws2_32)  # For networking if needed
endif()

# Strip release builds on macOS
if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_custom_command(TARGET pd_node_project POST_BUILD 
        COMMAND strip -x $<TARGET_FILE:pd_node_project>
        COMMENT "Stripping debug symbols from pd-node external"
    )
endif()

# Install target (optional)
install(TARGETS pd_node_project
    LIBRARY DESTINATION lib/pd/extra/pd-node
    RUNTIME DESTINATION lib/pd/extra/pd-node
)
install(DIRECTORY pd-api DESTINATION lib/pd/extra/pd-node)
install(FILES node/node-help.pd DESTINATION lib/pd/extra/pd-node)

# Print build summary
message(STATUS "===========================================")
message(STATUS "pd-node v${PROJECT_VERSION}")
message(STATUS "Target: ${TARGET_TRIPLET}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Output: ${PD_OUTPUT_PATH}")
message(STATUS "===========================================")
