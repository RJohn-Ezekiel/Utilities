#pragma once

#include "visio/error.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>

namespace visio::detail {

using FilePtr = std::unique_ptr<std::FILE, decltype(&pclose)>;

[[nodiscard]] inline FilePtr popen(const std::string& command, const char* mode)
{
    return FilePtr(::popen(command.c_str(), mode), &pclose);
}

[[nodiscard]] inline Result<std::string> exec(const std::string& command)
{
    auto pipe = FilePtr(::popen(command.c_str(), "r"), &::pclose);
    if (!pipe) {
        return makeError<std::string>(
            ErrorCode::SubprocessError,
            "failed to execute: " + command);
    }

    std::string result;
    std::array<char, 4096> buffer{};
    while (std::fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result.append(buffer.data());
    }

    auto exitCode = ::pclose(pipe.release());
    if (exitCode != 0) {
        return makeError<std::string>(
            ErrorCode::SubprocessError,
            std::format("command '{}' exited with code {}", command, exitCode));
    }

    return result;
}

/// Shell-quote a string so it's safe to embed in a single-quoted shell argument.
/// Replaces each ' with the sequence: '\'' (end-quote, literal quote, re-quote).
[[nodiscard]] inline std::string shellQuote(std::string_view s)
{
    std::string out;
    out.reserve(s.size() + 8);
    out += '\'';
    for (auto ch : s) {
        if (ch == '\'') {
            out += "'\\''";
        } else {
            out += ch;
        }
    }
    out += '\'';
    return out;
}

[[nodiscard]] inline Result<std::string> httpGet(std::string_view url)
{
    return exec(
        std::format("curl -s --compressed --http1.1 --keepalive-time 30 {}",
                     shellQuote(url)));
}

[[nodiscard]] inline Result<std::string> httpPost(
    std::string_view url,
    std::string_view body)
{
    auto tmpFile = std::format("/tmp/visio_post_{}.json", getpid());
    {
        std::ofstream ofs(tmpFile);
        if (ofs) ofs << body;
    }

    auto result = exec(
        std::format("curl -s --compressed --http1.1 "
                     "-H \"Content-Type: application/json\" "
                     "-d @{} "
                     "{}",
                     shellQuote(tmpFile), shellQuote(url)));

    std::remove(tmpFile.c_str());
    return result;
}

[[nodiscard]] inline Result<std::string> extractYtInitialData(
    std::string_view html)
{
    auto start = html.find("var ytInitialData = ");
    if (start == std::string_view::npos) {
        return makeError<std::string>(
            ErrorCode::ParseError,
            "could not find ytInitialData in response");
    }

    start += 20; // skip "var ytInitialData = "
    auto end = html.find(";</script>", start);
    if (end == std::string_view::npos) {
        return makeError<std::string>(
            ErrorCode::ParseError,
            "could not find end of ytInitialData");
    }

    return std::string(html.substr(start, end - start));
}

[[nodiscard]] inline std::string cacheDir()
{
    auto home = std::getenv("HOME");
    auto xdgCache = std::getenv("XDG_CACHE_HOME");
    auto base = xdgCache ? std::string(xdgCache) : (std::string(home ? home : "/tmp") + "/.cache");
    return base + "/visio";
}

[[nodiscard]] inline std::string configDir()
{
    auto home = std::getenv("HOME");
    auto xdgConfig = std::getenv("XDG_CONFIG_HOME");
    auto base = xdgConfig ? std::string(xdgConfig) : (std::string(home ? home : "/tmp") + "/.config");
    return base + "/visio";
}

[[nodiscard]] inline std::string urlEncode(std::string_view input)
{
    std::string result;
    result.reserve(input.size());
    for (unsigned char c : input) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            result += std::format("%{:02X}", static_cast<int>(c));
        }
    }
    return result;
}

} // namespace visio::detail
