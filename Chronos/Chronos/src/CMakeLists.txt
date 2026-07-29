add_subdirectory(core)
add_subdirectory(models)
add_subdirectory(storage)
add_subdirectory(services)
add_subdirectory(ui)
add_subdirectory(cli)

add_executable(chronos
    main.cpp
)

target_link_libraries(chronos PRIVATE
    chronos_core
    chronos_models
    chronos_storage
    chronos_services
    chronos_ui
    chronos_cli
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Multimedia
)

target_include_directories(chronos PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS chronos RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
