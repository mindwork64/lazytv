cmake_minimum_required(VERSION 3.21)
project(lgremote VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

qt_standard_project_setup()

qt_add_executable(lgremote
    src/main.cpp
    src/theme/Theme.cpp
    src/data/SessionStore.cpp
    src/data/AppContainer.cpp
    src/net/LgNetCastClient.cpp
    src/ui/MainWindow.cpp
    src/ui/RemoteScreen.cpp
    src/ui/PairingScreen.cpp
    src/ui/SettingsScreen.cpp
    src/ui/widgets/IconButton.cpp
    src/ui/widgets/StatusBar.cpp
    src/ui/widgets/DPad.cpp
    src/ui/widgets/RockerColumn.cpp
    src/ui/widgets/Keypad.cpp
    resources/resources.qrc
)

target_include_directories(lgremote PRIVATE src)

target_link_libraries(lgremote PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Svg
)