add_library(bible_core
    Bible.cpp
    Reference.cpp
)

target_sources(bible_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Verse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Chapter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Book.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Bible.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Reference.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SearchResult.h
)

target_include_directories(bible_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)
