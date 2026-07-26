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
find_package(Qt6 6.11 REQUIRED COMPONENTS
    Core
    Concurrent
    Gui
    Widgets
    Svg
    Network
    SerialPort
)

qt_add_executable(UpperComputer WIN32
    main.cpp
    resources/resources.qrc
    src/models/AppTypes.h
    src/models/ConnectionTypes.h
    src/models/ProtocolDefinition.h
    src/app/AppSettings.h
    src/app/AppSettings.cpp
    src/app/AppContext.h
    src/app/AppContext.cpp
    src/services/ConnectionManager.h
    src/services/ConnectionManager.cpp
    src/services/CommunicationCodec.h
    src/services/transports/AbstractTransport.h
    src/services/transports/AbstractTransport.cpp
    src/services/transports/SerialTransport.h
    src/services/transports/SerialTransport.cpp
    src/services/transports/NetworkTransports.h
    src/services/transports/NetworkTransports.cpp
    src/services/transports/VirtualTransport.h
    src/services/transports/VirtualTransport.cpp
    src/theme/ThemeManager.h
    src/theme/ThemeManager.cpp
    src/theme/IconManager.h
    src/theme/IconManager.cpp
    src/pages/PageId.h
    src/pages/PageRegistry.h
    src/pages/PageRegistry.cpp
    src/pages/PlotPage.h
    src/pages/PlotPage.cpp
    src/pages/CommunicationPage.h
    src/pages/CommunicationPage.cpp
    src/pages/communication/HardwareConfigPanel.h
    src/pages/communication/HardwareConfigPanel.cpp
    src/pages/communication/CommunicationModePanel.h
    src/pages/communication/CommunicationModePanel.cpp
    src/pages/communication/CommunicationMonitorPanel.h
    src/pages/communication/CommunicationMonitorPanel.cpp
    src/pages/communication/SendPanel.h
    src/pages/communication/SendPanel.cpp
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
    src/widgets/FocusUnderline.h
    src/widgets/FocusUnderline.cpp
    src/utils/ChecksumUtils.h
    src/utils/ChecksumUtils.cpp
    src/window/MainWindow.h
    src/window/MainWindow.cpp
)

target_link_libraries(UpperComputer PRIVATE
    Qt6::Core
    Qt6::Concurrent
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg
    Qt6::Network
    Qt6::SerialPort
)

target_include_directories(UpperComputer PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")

if(MSVC)
    target_compile_options(UpperComputer PRIVATE /W4 /utf-8 /permissive-)
endif()

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(ChecksumUtilsTest
        tests/ChecksumUtilsTest.cpp
        src/utils/ChecksumUtils.h
        src/utils/ChecksumUtils.cpp
        src/models/ConnectionTypes.h
    )
    target_include_directories(ChecksumUtilsTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ChecksumUtilsTest PRIVATE
        Qt6::Core
        Qt6::SerialPort
    )
    if(MSVC)
        target_compile_options(ChecksumUtilsTest PRIVATE /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME ChecksumUtilsTest COMMAND ChecksumUtilsTest)

    qt_add_executable(ConnectionManagerIntegrationTest
        tests/ConnectionManagerIntegrationTest.cpp
        src/models/AppTypes.h
        src/models/ConnectionTypes.h
        src/services/ConnectionManager.h
        src/services/ConnectionManager.cpp
        src/services/transports/AbstractTransport.h
        src/services/transports/AbstractTransport.cpp
        src/services/transports/SerialTransport.h
        src/services/transports/SerialTransport.cpp
        src/services/transports/NetworkTransports.h
        src/services/transports/NetworkTransports.cpp
        src/services/transports/VirtualTransport.h
        src/services/transports/VirtualTransport.cpp
    )
    target_include_directories(ConnectionManagerIntegrationTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ConnectionManagerIntegrationTest PRIVATE
        Qt6::Core
        Qt6::Network
        Qt6::SerialPort
    )
    if(MSVC)
        target_compile_options(ConnectionManagerIntegrationTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME ConnectionManagerIntegrationTest
        COMMAND ConnectionManagerIntegrationTest)

    qt_add_executable(AppSettingsPersistenceTest
        tests/AppSettingsPersistenceTest.cpp
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/models/AppTypes.h
        src/models/ConnectionTypes.h
        src/pages/PageId.h
    )
    target_include_directories(AppSettingsPersistenceTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(AppSettingsPersistenceTest PRIVATE
        Qt6::Core
        Qt6::SerialPort
    )
    if(MSVC)
        target_compile_options(AppSettingsPersistenceTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME AppSettingsPersistenceTest
        COMMAND AppSettingsPersistenceTest)

    qt_add_executable(CommunicationPageSmokeTest
        tests/CommunicationPageSmokeTest.cpp
        resources/resources.qrc
        src/models/AppTypes.h
        src/models/ConnectionTypes.h
        src/models/ProtocolDefinition.h
        src/pages/PageId.h
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/app/AppContext.h
        src/app/AppContext.cpp
        src/services/ConnectionManager.h
        src/services/ConnectionManager.cpp
        src/services/CommunicationCodec.h
        src/services/transports/AbstractTransport.h
        src/services/transports/AbstractTransport.cpp
        src/services/transports/SerialTransport.h
        src/services/transports/SerialTransport.cpp
        src/services/transports/NetworkTransports.h
        src/services/transports/NetworkTransports.cpp
        src/services/transports/VirtualTransport.h
        src/services/transports/VirtualTransport.cpp
        src/theme/ThemeManager.h
        src/theme/ThemeManager.cpp
        src/theme/IconManager.h
        src/theme/IconManager.cpp
        src/widgets/FocusUnderline.h
        src/widgets/FocusUnderline.cpp
        src/utils/ChecksumUtils.h
        src/utils/ChecksumUtils.cpp
        src/pages/CommunicationPage.h
        src/pages/CommunicationPage.cpp
        src/pages/communication/HardwareConfigPanel.h
        src/pages/communication/HardwareConfigPanel.cpp
        src/pages/communication/CommunicationModePanel.h
        src/pages/communication/CommunicationModePanel.cpp
        src/pages/communication/CommunicationMonitorPanel.h
        src/pages/communication/CommunicationMonitorPanel.cpp
        src/pages/communication/SendPanel.h
        src/pages/communication/SendPanel.cpp
    )
    target_include_directories(CommunicationPageSmokeTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(CommunicationPageSmokeTest PRIVATE
        Qt6::Core
        Qt6::Concurrent
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::Network
        Qt6::SerialPort
    )
    if(MSVC)
        target_compile_options(CommunicationPageSmokeTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME CommunicationPageSmokeTest
        COMMAND CommunicationPageSmokeTest)
    if(WIN32)
        add_custom_command(TARGET CommunicationPageSmokeTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:CommunicationPageSmokeTest>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
                    "$<TARGET_FILE_DIR:CommunicationPageSmokeTest>/platforms"
            COMMENT "Deploying the offscreen Qt platform for UI smoke tests"
        )
    endif()
endif()

# Keep the build output directly runnable outside CLion. copy_if_different
# avoids rewriting large Qt DLLs after every incremental link.
if(WIN32)
    add_custom_command(TARGET UpperComputer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:UpperComputer>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::Core>"
                "$<TARGET_FILE:Qt6::Concurrent>"
                "$<TARGET_FILE:Qt6::Gui>"
                "$<TARGET_FILE:Qt6::Widgets>"
                "$<TARGET_FILE:Qt6::Svg>"
                "$<TARGET_FILE:Qt6::Network>"
                "$<TARGET_FILE:Qt6::SerialPort>"
                "$<TARGET_FILE_DIR:UpperComputer>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<TARGET_FILE:Qt6::QWindowsIntegrationPlugin>"
                "$<TARGET_FILE_DIR:UpperComputer>/platforms"
        COMMENT "Deploying the minimal Qt runtime"
    )
endif()
