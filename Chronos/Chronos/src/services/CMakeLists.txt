add_library(chronos_services STATIC
    TimerService.cpp
    AudioService.cpp
    TaskService.cpp
    ReminderScheduler.cpp
    NotificationService.cpp
    StatisticsService.cpp
)

target_include_directories(chronos_services PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chronos_services PUBLIC
    chronos_core
    chronos_models
    chronos_storage
    Qt6::Core
    Qt6::Multimedia
    Qt6::Widgets
)
