#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

class NoteStorage {
public:
    NoteStorage();

    void setNote(std::string refKey, std::string text);
    [[nodiscard]] std::string note(std::string_view refKey) const;
    void deleteNote(std::string_view refKey);
    [[nodiscard]] bool hasNote(std::string_view refKey) const;

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> allNotes() const;

    void save();
    void load();

private:
    [[nodiscard]] std::filesystem::path filePath() const;

    std::map<std::string, std::string, std::less<>> notes_;
};
