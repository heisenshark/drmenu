cmake_minimum_required(VERSION 3.16)
project(drmenu VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable automatic Qt compiler wrapper generation (signals, slots, resources)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Network)
find_package(LayerShellQt REQUIRED)

# Modern Qt6 configuration helper
qt_standard_project_setup()

# Define executable and sources
qt_add_executable(drmenu
    src/main.cpp
    src/cli_parser.cpp
    src/screen_detector.cpp
    src/output_controller.cpp
    src/daemon.cpp
    src/client.cpp
    src/standalone_app.cpp
)

# Place binary in a 'bin' subdirectory to avoid conflicts with QML module directories
set_target_properties(drmenu PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Define QML module
qt_add_qml_module(drmenu
    URI drmenu
    VERSION 1.0
    QML_FILES
        src/qml/main.qml
)

# Link Qt6 libraries and LayerShellQt
target_link_libraries(drmenu PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::Network
    LayerShellQt::Interface
)
