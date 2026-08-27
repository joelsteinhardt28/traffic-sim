cmake_minimum_required(VERSION 3.10)
project(TrafficSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find SFML package
find_package(SFML 2.5 COMPONENTS graphics window system REQUIRED)

# Add your executable and source files
add_executable(TrafficSim
    src/main.cpp
    src/kdTree.cpp
    src/curves.cpp
    src/Agent.cpp
    src/toolbox.cpp
    src/RoadNetwork.cpp
    src/NetworkManager.cpp
    src/CurveViewer.cpp
    src/RoadViewer.cpp
)

# Target include directories
target_include_directories(TrafficSim PRIVATE include)

# Link SFML libraries
target_link_libraries(TrafficSim sfml-graphics sfml-window sfml-system)