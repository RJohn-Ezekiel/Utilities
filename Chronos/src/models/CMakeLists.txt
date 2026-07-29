add_library(chronos_models INTERFACE)
target_include_directories(chronos_models INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(chronos_models INTERFACE chronos_core Qt6::Core)
