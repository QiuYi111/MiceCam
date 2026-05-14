add_executable(micecam_ui main.cpp MockCameraModel.cpp)

qt_policy(SET QTP0004 NEW)

qt_add_qml_module(micecam_ui
    URI "MiceCam.UI"
    VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        qml/main.qml
        qml/theme/Theme.qml
        qml/components/AppTitleBar.qml
        qml/components/AppSidebar.qml
        qml/components/AppStatusBar.qml
        qml/components/AppIcon.qml
        qml/components/AppToolbar.qml
        qml/components/CameraCard.qml
        qml/components/CameraGridView.qml
        qml/components/EncodingSettings.qml
        qml/components/AlertsSettings.qml
        qml/components/OutputSettings.qml
        qml/components/LoggingSettings.qml
        qml/components/AboutView.qml
        qml/components/NotificationPopup.qml
    RESOURCES
        qml/theme/qmldir
        resources/logo.png
        resources/banner.png
)

target_include_directories(micecam_ui PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/api
    ${CMAKE_SOURCE_DIR}/internal
)

target_link_libraries(micecam_ui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
)

target_compile_options(micecam_ui PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

set_target_properties(micecam_ui PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/cmd/micecam_ui"
)
