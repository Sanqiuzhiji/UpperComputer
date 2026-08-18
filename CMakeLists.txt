cmake_minimum_required(VERSION 3.24)
project(UpperComputer VERSION 0.1.0 LANGUAGES CXX)

option(UPPERCOMPUTER_PORTABLE_BUILD
    "Store settings and workspaces beside the executable" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
add_compile_definitions(
    UPPERCOMPUTER_PROJECT_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

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

set(TELEMETRY_SOURCES
    src/models/TelemetryTypes.h
    src/services/ChannelDataHub.h
    src/services/ChannelDataHub.cpp
    src/services/ReceiveDataPipeline.h
    src/services/ReceiveDataPipeline.cpp
)

set(PLOT_SOURCES
    src/pages/plot/PlotTypes.h
    src/pages/plot/PlotTheme.h
    src/pages/plot/PlotToolBar.h
    src/pages/plot/PlotToolBar.cpp
    src/pages/plot/DetachableTabBar.h
    src/pages/plot/DetachableTabBar.cpp
    src/pages/plot/DetachableTabWidget.h
    src/pages/plot/DetachableTabWidget.cpp
    src/pages/plot/PlotFloatingWindow.h
    src/pages/plot/PlotFloatingWindow.cpp
    src/pages/plot/PlotWorkspacePage.h
    src/pages/plot/PlotWorkspacePage.cpp
    src/pages/plot/PlotCanvas.h
    src/pages/plot/PlotCanvas.cpp
    src/pages/plot/DashboardItem.h
    src/pages/plot/DashboardItem.cpp
    src/pages/plot/RealtimePlotWidget.h
    src/pages/plot/RealtimePlotWidget.cpp
    src/pages/plot/PlotChannelDialog.h
    src/pages/plot/PlotChannelDialog.cpp
)

qt_add_executable(UpperComputer WIN32
    main.cpp
    resources/resources.qrc
    src/models/AppTypes.h
    src/models/ConnectionTypes.h
    src/models/ProtocolDefinition.h
    src/models/ProtocolTypes.h
    src/models/ProtocolTypes.cpp
    src/app/AppSettings.h
    src/app/AppSettings.cpp
    src/app/AppContext.h
    src/app/AppContext.cpp
    src/services/ConnectionManager.h
    src/services/ConnectionManager.cpp
    src/services/CommunicationCodec.h
    src/services/CustomBinaryCodec.h
    src/services/CustomBinaryCodec.cpp
    src/services/ProtocolRepository.h
    src/services/ProtocolRepository.cpp
    ${TELEMETRY_SOURCES}
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
    ${PLOT_SOURCES}
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
    src/pages/protocol/ProtocolMime.h
    src/pages/protocol/FieldLibraryPanel.h
    src/pages/protocol/FieldLibraryPanel.cpp
    src/pages/protocol/ProtocolCanvas.h
    src/pages/protocol/ProtocolCanvas.cpp
    src/pages/protocol/ProtocolFrameWidget.h
    src/pages/protocol/ProtocolFrameWidget.cpp
    src/pages/protocol/ProtocolFieldCard.h
    src/pages/protocol/ProtocolFieldCard.cpp
    src/pages/protocol/ProtocolPropertyPanel.h
    src/pages/protocol/ProtocolPropertyPanel.cpp
    src/pages/CanBusPage.h
    src/pages/CanBusPage.cpp
    src/pages/CescToolPage.h
    src/pages/CescToolPage.cpp
    src/services/CescFirmwareUploader.h
    src/services/CescFirmwareUploader.cpp
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

if(UPPERCOMPUTER_PORTABLE_BUILD)
    target_compile_definitions(UpperComputer PRIVATE
        UPPERCOMPUTER_PORTABLE_BUILD=1)
endif()

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
        src/models/ProtocolTypes.h
        src/models/ProtocolTypes.cpp
        src/pages/PageId.h
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/app/AppContext.h
        src/app/AppContext.cpp
        src/services/ConnectionManager.h
        src/services/ConnectionManager.cpp
        src/services/CommunicationCodec.h
        src/services/CustomBinaryCodec.h
        src/services/CustomBinaryCodec.cpp
        src/services/ProtocolRepository.h
        src/services/ProtocolRepository.cpp
        ${TELEMETRY_SOURCES}
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

    qt_add_executable(ProtocolEditorTest
        tests/ProtocolEditorTest.cpp
        resources/resources.qrc
        src/models/AppTypes.h
        src/models/ConnectionTypes.h
        src/models/ProtocolDefinition.h
        src/models/ProtocolTypes.h
        src/models/ProtocolTypes.cpp
        src/pages/PageId.h
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/app/AppContext.h
        src/app/AppContext.cpp
        src/services/ConnectionManager.h
        src/services/ConnectionManager.cpp
        src/services/CommunicationCodec.h
        src/services/CustomBinaryCodec.h
        src/services/CustomBinaryCodec.cpp
        src/services/ProtocolRepository.h
        src/services/ProtocolRepository.cpp
        ${TELEMETRY_SOURCES}
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
        src/utils/ChecksumUtils.h
        src/utils/ChecksumUtils.cpp
        src/pages/ProtocolEditorPage.h
        src/pages/ProtocolEditorPage.cpp
        src/pages/protocol/ProtocolMime.h
        src/pages/protocol/FieldLibraryPanel.h
        src/pages/protocol/FieldLibraryPanel.cpp
        src/pages/protocol/ProtocolCanvas.h
        src/pages/protocol/ProtocolCanvas.cpp
        src/pages/protocol/ProtocolFrameWidget.h
        src/pages/protocol/ProtocolFrameWidget.cpp
        src/pages/protocol/ProtocolFieldCard.h
        src/pages/protocol/ProtocolFieldCard.cpp
        src/pages/protocol/ProtocolPropertyPanel.h
        src/pages/protocol/ProtocolPropertyPanel.cpp
    )
    target_include_directories(ProtocolEditorTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ProtocolEditorTest PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Svg
        Qt6::Network
        Qt6::SerialPort
    )
    if(MSVC)
        target_compile_options(ProtocolEditorTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME ProtocolEditorTest COMMAND ProtocolEditorTest)
    if(WIN32)
        add_custom_command(TARGET ProtocolEditorTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:ProtocolEditorTest>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
                    "$<TARGET_FILE_DIR:ProtocolEditorTest>/platforms"
            COMMENT "Deploying the offscreen Qt platform for protocol tests"
        )
    endif()

    qt_add_executable(ChannelDataHubTest
        tests/ChannelDataHubTest.cpp
        src/models/ProtocolDefinition.h
        src/models/TelemetryTypes.h
        src/services/ChannelDataHub.h
        src/services/ChannelDataHub.cpp
    )
    target_include_directories(ChannelDataHubTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(ChannelDataHubTest PRIVATE Qt6::Core Qt6::Gui)
    if(MSVC)
        target_compile_options(ChannelDataHubTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME ChannelDataHubTest COMMAND ChannelDataHubTest)

    qt_add_executable(PlotPageSmokeTest
        tests/PlotPageSmokeTest.cpp
        resources/resources.qrc
        src/models/AppTypes.h
        src/models/ConnectionTypes.h
        src/models/ProtocolDefinition.h
        src/models/ProtocolTypes.h
        src/models/ProtocolTypes.cpp
        src/pages/PageId.h
        src/app/AppSettings.h
        src/app/AppSettings.cpp
        src/app/AppContext.h
        src/app/AppContext.cpp
        src/services/ConnectionManager.h
        src/services/ConnectionManager.cpp
        src/services/CommunicationCodec.h
        src/services/CustomBinaryCodec.h
        src/services/CustomBinaryCodec.cpp
        src/services/ProtocolRepository.h
        src/services/ProtocolRepository.cpp
        ${TELEMETRY_SOURCES}
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
        src/utils/ChecksumUtils.h
        src/utils/ChecksumUtils.cpp
        src/pages/PlotPage.h
        src/pages/PlotPage.cpp
        ${PLOT_SOURCES}
    )
    target_include_directories(PlotPageSmokeTest PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(PlotPageSmokeTest PRIVATE
        Qt6::Core Qt6::Concurrent Qt6::Gui Qt6::Widgets Qt6::Svg
        Qt6::Network Qt6::SerialPort)
    if(MSVC)
        target_compile_options(PlotPageSmokeTest PRIVATE
            /W4 /utf-8 /permissive-)
    endif()
    add_test(NAME PlotPageSmokeTest COMMAND PlotPageSmokeTest)
    if(WIN32)
        add_custom_command(TARGET PlotPageSmokeTest POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory
                    "$<TARGET_FILE_DIR:PlotPageSmokeTest>/platforms"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:Qt6::QOffscreenIntegrationPlugin>"
                    "$<TARGET_FILE_DIR:PlotPageSmokeTest>/platforms"
            COMMENT "Deploying the offscreen Qt platform for plot tests"
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
