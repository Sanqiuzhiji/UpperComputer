cmake_minimum_required(VERSION 3.24)
project(UpperComputer VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# Configure Qt6_DIR or CMAKE_PREFIX_PATH in the CLion CMake profile/toolchain.
find_package(Qt6 6.11 REQUIRED COMPONENTS Core Gui Widgets Svg)

qt_add_executable(UpperComputer WIN32
    main.cpp
    resources/resources.qrc
    src/models/AppTypes.h
    src/app/AppSettings.h
    src/app/AppSettings.cpp
    src/app/AppContext.h
    src/app/AppContext.cpp
    src/services/ConnectionManager.h
    src/services/ConnectionManager.cpp
    src/theme/ThemeManager.h
    src/theme/ThemeManager.cpp
    src/theme/IconManager.h
    src/theme/IconManager.cpp
    src/pages/PageId.h
    src/pages/PageRegistry.h
    src/pages/PageRegistry.cpp
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
    src/widgets/ThemeTransitionOverlay.h
    src/widgets/ThemeTransitionOverlay.cpp
    src/widgets/ToastWidget.h
    src/widgets/ToastWidget.cpp
    src/window/MainWindow.h
    src/window/MainWindow.cpp
)

target_link_libraries(UpperComputer PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
)

target_include_directories(UpperComputer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(UpperComputer PRIVATE /W4 /utf-8 /permissive-)
endif()

# Keep the build output directly runnable outside CLion. copy_if_different
# avoids rewriting large Qt DLLs after every incremental link.
if(WIN32)
    add_custom_command(TARGET UpperComputer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:UpperComputer>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::Core>"
                "$<TARGET_FILE:Qt6::Gui>"
                "$<TARGET_FILE:Qt6::Widgets>"
                "$<TARGET_FILE:Qt6::Svg>"
                "$<TARGET_FILE_DIR:UpperComputer>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
                "$<TARGET_FILE_DIR:UpperComputer>/platforms"
        COMMENT "Deploying the minimal Qt runtime"
    )
endif()
