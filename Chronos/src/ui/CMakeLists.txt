add_library(chronos_ui STATIC
    MainWindow.cpp
    CircularTimerWidget.cpp
    ToolbarWidget.cpp
    SidebarWidget.cpp
    StatusBarWidget.cpp
    TaskWidget.cpp
    TaskListWidget.cpp
    DashboardWidget.cpp
    StatisticsWidget.cpp
    HistoryWidget.cpp
    SessionCompleteDialog.cpp
    ToastNotification.cpp
    MiniModeWindow.cpp
    SettingsWidget.cpp
    WaterMeterWidget.cpp
)

target_include_directories(chronos_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chronos_ui PUBLIC
    chronos_core
    chronos_models
    chronos_services
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)
