cmake_minimum_required(VERSION 3.21)
project(lazytv VERSION 1.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

# Опция: устанавливать библиотечные файлы (заголовки, .a, CMake config).
# Для PPA-пакета выключается, чтобы в .deb попал только исполняемый файл.
option(LAZYTV_INSTALL_LIB
    "Install library files (headers, static libs, CMake configs)"
    ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Svg)

qt_standard_project_setup()

# =====================================================================
# liblazytv-core — протокол LG NetCast, без UI и без persistence
# =====================================================================
add_library(lazytv-core STATIC
    src/net/NetCastClient.cpp
    include/lazytv/client.hpp
    include/lazytv/lazytv_export.h
)
add_library(LazyTV::Core ALIAS lazytv-core)

set_target_properties(lazytv-core PROPERTIES EXPORT_NAME Core)

target_include_directories(lazytv-core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(lazytv-core
    PUBLIC
        Qt6::Core
        Qt6::Network
)

# =====================================================================
# liblazytv — core + SessionStore + AppContainer
# =====================================================================
add_library(lazytv STATIC
    src/data/SessionStore.cpp
    src/data/AppContainer.cpp
    include/lazytv/session_store.hpp
    include/lazytv/app_container.hpp
    include/lazytv/lazytv.h
)
add_library(LazyTV::Client ALIAS lazytv)

set_target_properties(lazytv PROPERTIES EXPORT_NAME Client)

target_link_libraries(lazytv
    PUBLIC lazytv-core
)

# =====================================================================
# Приложение
# =====================================================================
qt_add_executable(lazytv-app
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

target_include_directories(lazytv-app PRIVATE src)

target_link_libraries(lazytv-app PRIVATE
    lazytv
    Qt6::Widgets
    Qt6::Svg
)

set_target_properties(lazytv-app PROPERTIES OUTPUT_NAME lazytv)

# Прокидываем версию из CMake в код — доступна как LAZYTV_VERSION
target_compile_definitions(lazytv-app PRIVATE
    LAZYTV_VERSION="${PROJECT_VERSION}"
)

# =====================================================================
# Установка библиотеки (опционально)
# =====================================================================
if(LAZYTV_INSTALL_LIB)
    include(CMakePackageConfigHelpers)

    install(TARGETS lazytv-core lazytv
        EXPORT LazyTVTargets
        ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(DIRECTORY include/lazytv
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )

    install(EXPORT LazyTVTargets
        FILE LazyTVTargets.cmake
        NAMESPACE LazyTV::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LazyTV
    )

    configure_package_config_file(
        cmake/LazyTVConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/LazyTVConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LazyTV
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/LazyTVConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/LazyTVConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/LazyTVConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/LazyTV
    )
endif()

# =====================================================================
# Установка приложения (всегда)
# =====================================================================
install(TARGETS lazytv-app
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES lazytv.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES resources/ic_launcher.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME lazytv.svg
)