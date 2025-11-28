#include "../include/GUI.h"
#include "../include/Port.h"
#include <filesystem>
#include <iostream>

namespace {
std::filesystem::path getExecutableDir(char *argv0) {
    std::error_code ec;
    auto exePath = std::filesystem::canonical(std::filesystem::path(argv0), ec);
    if (ec) {
        exePath = std::filesystem::absolute(std::filesystem::path(argv0), ec);
    }
    return exePath.has_parent_path() ? exePath.parent_path() : std::filesystem::current_path();
}

std::string resolveResource(const std::string &relative, const std::filesystem::path &exeDir) {
    std::filesystem::path relPath(relative);
    if (relPath.is_absolute()) {
        return relPath.string();
    }

    for (const auto &base : {std::filesystem::current_path(), exeDir, exeDir.parent_path()}) {
        if (base.empty()) {
            continue;
        }
        std::error_code ec;
        auto candidate = base / relPath;
        if (std::filesystem::exists(candidate, ec)) {
            return candidate.string();
        }
    }

    return relPath.string();
}
} // namespace

int main(int /*argc*/, char **argv) {
    try {
        const auto exeDir = getExecutableDir(argv[0]);
        const std::string settingsFile = resolveResource("../settings.json", exeDir);
        const std::string scheduleFile = resolveResource("../schedule.json", exeDir);
        const std::string fontPath = resolveResource("../fonts/DejaVuSans.ttf", exeDir);

        Port port(settingsFile, scheduleFile);
        port.process();

        PortGUI gui(port, settingsFile, fontPath);
        gui.run();
    } catch (const std::exception &ex) {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return 1;
    }
    return 0;
}
