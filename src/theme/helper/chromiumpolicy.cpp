#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace fs = std::filesystem;

namespace {

constexpr std::array k_policyDirs = {
    std::string_view{ "/etc/chromium/policies/managed" },
    std::string_view{ "/etc/brave/policies/managed" },
    std::string_view{ "/etc/opt/chrome/policies/managed" },
};

bool isHexColour(std::string_view value) {
    return value.size() == 6 && std::ranges::all_of(value, [](char c) {
        return std::isxdigit(static_cast<unsigned char>(c)) != 0;
    });
}

bool writePolicy(const fs::path& dir, std::string_view colour) {
    std::error_code ec;
    fs::create_directories(dir, ec);
    if (ec) {
        std::fprintf(stderr, "Unable to create %s: %s\n", dir.c_str(), ec.message().c_str());
        return false;
    }

    const auto path = dir / "caelestia.json";
    const auto tmp = dir / "caelestia.json.tmp";
    {
        std::ofstream out(tmp, std::ios::trunc);
        out << R"({"BrowserThemeColor":"#)" << colour << R"(","BrowserColorScheme":"device"})" << '\n';
        if (!out) {
            std::fprintf(stderr, "Unable to write %s\n", tmp.c_str());
            return false;
        }
    }
    fs::permissions(
        tmp, fs::perms::owner_write | fs::perms::owner_read | fs::perms::group_read | fs::perms::others_read, ec);
    fs::rename(tmp, path, ec);
    if (ec) {
        std::fprintf(stderr, "Unable to write %s: %s\n", path.c_str(), ec.message().c_str());
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc != 2 || !isHexColour(argv[1])) {
        std::fprintf(stderr, "Usage: %s <rrggbb>\n", argc > 0 ? argv[0] : "caelestia-chromium-policy");
        return EXIT_FAILURE;
    }

    std::string colour = argv[1];
    for (auto& c : colour)
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    bool ok = true;
    for (const auto dir : k_policyDirs)
        ok = writePolicy(fs::path(dir), colour) && ok;
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
