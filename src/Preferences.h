#pragma once
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <utility>
namespace SideKick {
inline std::filesystem::path preferencesPath() {
#ifdef _WIN32
    const wchar_t *root = _wgetenv(L"APPDATA");
    return root ? std::filesystem::path(root) / "SideKick" / "window.txt" : std::filesystem::path{};
#elif defined(__APPLE__)
    const char *root = std::getenv("HOME");
    return root ? std::filesystem::u8path(root) / "Library/Application Support/SideKick/window.txt"
                : std::filesystem::path{};
#else
    const char *root = std::getenv("XDG_CONFIG_HOME");
    if (root)
        return std::filesystem::u8path(root) / "SideKick/window.txt";
    root = std::getenv("HOME");
    return root ? std::filesystem::u8path(root) / ".config/SideKick/window.txt"
                : std::filesystem::path{};
#endif
}
inline std::pair<int, int> loadWindowSize(const std::filesystem::path &path = preferencesPath()) {
    int w = 698, h = 478;
    std::ifstream in(path);
    int a, b;
    if (in >> a >> b && a >= 640 && a <= 2400 && b >= 460 && b <= 1800) {
        w = a;
        h = b;
    }
    return {w, h};
}
inline void saveWindowSize(int w, int h, const std::filesystem::path &path = preferencesPath()) {
    // Called from the UI thread only, after resize settles. Never from process().
    if (w < 640 || w > 2400 || h < 460 || h > 1800)
        return;
    auto p = path;
    if (p.empty())
        return;
    std::error_code ec;
    std::filesystem::create_directories(p.parent_path(), ec);
    if (ec)
        return;
    auto temp = p;
    temp += ".tmp";
    {
        std::ofstream out(temp);
        if (!out)
            return;
        out << w << ' ' << h << '\n';
        out.flush();
        if (!out)
            return;
    }
#ifdef _WIN32
    // Rename-over-existing is not supported by the Windows filesystem API.
    std::filesystem::remove(p, ec);
    ec.clear();
#endif
    std::filesystem::rename(temp, p, ec);
}
} // namespace SideKick
