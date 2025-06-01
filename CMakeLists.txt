cmake_minimum_required(VERSION 3.16)
project(dxdiag_gui_app)

set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 6.8.1 COMPONENTS Widgets Network REQUIRED)

set(CMAKE_AUTOMOC ON)
set_source_files_properties(main.cpp PROPERTIES QT_AUTOMOC ON)

add_executable(dxdiag_gui_app main.cpp DxDiagWorker.cpp GameRequirementsWorker.cpp)

# Link Qt libraries
if (WIN32)
    target_link_libraries(dxdiag_gui_app PRIVATE Qt6::Widgets Qt6::Network)
endif()

# Include current directory for dxtextmake.h
include_directories(${CMAKE_CURRENT_SOURCE_DIR}) 