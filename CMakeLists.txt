cmake_minimum_required(VERSION 3.24)
project(UpperComputer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Prefer passing Qt6_DIR or CMAKE_PREFIX_PATH from the CLion toolchain/profile.
# Keep the local fallback so the existing workstation remains zero-config.
if(NOT DEFINED CMAKE_PREFIX_PATH AND
   EXISTS "D:/Professional_Use_Software/Qt/6.11.1/msvc2022_64")
    list(APPEND CMAKE_PREFIX_PATH
         "D:/Professional_Use_Software/Qt/6.11.1/msvc2022_64")
endif()

find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(UpperComputer WIN32
    main.cpp
    src/theme/ThemeManager.h
    src/theme/ThemeManager.cpp
    src/pages/PageId.h
    src/pages/PlotPage.h
    src/pages/PlotPage.cpp
    src/pages/ConnectionPage.h
    src/pages/ConnectionPage.cpp
    src/pages/ProtocolEditorPage.h
    src/pages/ProtocolEditorPage.cpp
    src/pages/CanBusPage.h
    src/pages/CanBusPage.cpp
    src/pages/MdfViewerPage.h
    src/pages/MdfViewerPage.cpp
    src/pages/AboutPage.h
    src/pages/AboutPage.cpp
    src/pages/SettingsPage.h
    src/pages/SettingsPage.cpp
    src/widgets/TitleBar.h
    src/widgets/TitleBar.cpp
    src/widgets/SideNavigation.h
    src/widgets/SideNavigation.cpp
    src/widgets/StatusBarWidget.h
    src/widgets/StatusBarWidget.cpp
    src/window/MainWindow.h
    src/window/MainWindow.cpp
)

target_link_libraries(UpperComputer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_include_directories(UpperComputer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(UpperComputer PRIVATE /W4 /utf-8 /permissive-)
endif()

# Deployment belongs to packaging. During development, CLion injects the Qt
# runtime path, avoiding a ~70 MiB DLL copy after every link.
