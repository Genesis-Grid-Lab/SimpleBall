cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# build output directories

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY $<0:>${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<0:>${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY $<0:>${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PDB_OUTPUT_DIRECTORY $<0:>${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH $<0:>${CMAKE_BINARY_DIR}/bin)

# cpp standard options

set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD 20)

set(SUPPORT_FILEFORMAT_HDR ON CACHE BOOL "Support loading HDR as textures" FORCE)
add_subdirectory(vendor/raylib)
add_subdirectory(vendor/entt)
add_subdirectory(vendor/lua)
add_subdirectory(vendor/sol2)
add_subdirectory(vendor/jolt/Build)

project(SimpleBall C CXX)

# gather all source files
file(GLOB_RECURSE sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB_RECURSE headers ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)

file(GLOB rlImGuiSRC ${CMAKE_SOURCE_DIR}/vendor/rlImGui/rlImGui.cpp)
file(GLOB rlImGuiHEAD ${CMAKE_SOURCE_DIR}/vendor/rlImGui/*.h)

file(GLOB ImGuiSRC ${CMAKE_SOURCE_DIR}/vendor/ImGui/*.cpp
${CMAKE_SOURCE_DIR}/vendor/ImGui/misc/cpp/*.cpp)

file(GLOB raylib-gizmo ${CMAKE_SOURCE_DIR}/vendor/raylib-gizmo/src/*.c)

add_executable(${PROJECT_NAME} ${sources} ${rlImGuiSRC} ${ImGuiSRC} ${raylib-gizmo} ${headers})

# export engine symbols
target_compile_definitions(${PROJECT_NAME} PUBLIC
)

# includes directories
target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/vendor/raylib/src
  ${CMAKE_SOURCE_DIR}/vendor/entt/single_include/entt
  ${CMAKE_SOURCE_DIR}/vendor/ImGui
  ${CMAKE_SOURCE_DIR}/vendor/ImGui/misc/cpp
  ${CMAKE_SOURCE_DIR}/vendor/rlImGui
  ${CMAKE_SOURCE_DIR}/vendor/raylib-gizmo/src
  ${CMAKE_SOURCE_DIR}/vendor/sol2/include
  ${CMAKE_SOURCE_DIR}/vendor/lua/src
  ${CMAKE_SOURCE_DIR}/vendor/jolt/Jolt
  ${CMAKE_SOURCE_DIR}/vendor/jolt
)

# link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC             
  raylib
  EnTT
  sol2
  liblua
  Jolt
)

# copy resources
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND 
        ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_SOURCE_DIR}/Resources
        ${EXECUTABLE_OUTPUT_PATH}/Resources
    )
else()
    message(WARNING "[WARNING] no resource directory!")
endif()

# copy data
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Data)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND 
        ${CMAKE_COMMAND} -E copy_directory 
        ${CMAKE_CURRENT_SOURCE_DIR}/Data
        ${EXECUTABLE_OUTPUT_PATH}/Data
    )
else()
    message(WARNING "[WARNING] no data directory!")
endif()
