find_package(Qt6 REQUIRED COMPONENTS Core)

add_library(bible_storage
    BookmarkStorage.cpp
    NoteStorage.cpp
    HistoryStorage.cpp
)

target_include_directories(bible_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bible_storage PUBLIC bible_core Qt6::Core)
