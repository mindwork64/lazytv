cmake_minimum_required(VERSION 3.21)
project(lgremote VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

qt_standard_project_setup()

# =====================================================================
# liblgremote-core — протокол LG NetCast, без UI и без persistence
# =====================================================================
add_library(lgremote-core STATIC
    src/net/LgNetCastClient.cpp
    include/lgremote/client.hpp
    include/lgremote/lgremote_export.h
)
add_library(LgRemote::Core ALIAS lgremote-core)

set_target_properties(lgremote-core PROPERTIES EXPORT_NAME Core)

target_include_directories(lgremote-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(lgremote-core
    PUBLIC
        Qt6::Core
        Qt6::Network
)

# =====================================================================
# liblgremote — core + SessionStore + AppContainer
# =====================================================================
add_library(lgremote STATIC
    src/data/SessionStore.cpp
    src/data/AppContainer.cpp
    include/lgremote/session_store.hpp
    include/lgremote/app_container.hpp
    include/lgremote/lgremote.h
)
add_library(LgRemote::Client ALIAS lgremote)

set_target_properties(lgremote PROPERTIES EXPORT_NAME Client)

target_link_libraries(lgremote
    PUBLIC lgremote-core
)

# =====================================================================
# Приложение
# =====================================================================
qt_add_executable(lgremote-app
    src/main.cpp
    src/theme/Theme.cpp
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

target_include_directories(lgremote-app PRIVATE src)

target_link_libraries(lgremote-app PRIVATE
    lgremote
    Qt6::Widgets
    Qt6::Svg
)

# Имя выходного файла — lgremote (без -app)
set_target_properties(lgremote-app PROPERTIES OUTPUT_NAME lgremote)

# =====================================================================
# Установка библиотек
# =====================================================================
include(CMakePackageConfigHelpers)

install(TARGETS lgremote-core lgremote
    EXPORT LgRemoteTargets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/lgremote
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT LgRemoteTargets
    FILE LgRemoteTargets.cmake
    NAMESPACE LgRemote::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LgRemote
)

configure_package_config_file(
    cmake/LgRemoteConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/LgRemoteConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LgRemote
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/LgRemoteConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/LgRemoteConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/LgRemoteConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LgRemote
)

# =====================================================================
# Установка приложения (для PPA / системной установки)
# =====================================================================
install(TARGETS lgremote-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES lgremote.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES resources/ic_launcher.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME lgremote.svg
)