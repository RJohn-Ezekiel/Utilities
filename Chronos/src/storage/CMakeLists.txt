add_library(chronos_storage STATIC
    StorageManager.cpp
)

target_include_directories(chronos_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(chronos_storage PUBLIC chronos_models Qt6::Core)
