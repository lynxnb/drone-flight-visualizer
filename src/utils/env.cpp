#include "env.h"

#include <filesystem>
#include <fstream>
#include <vector>

#include "exepath.h"

namespace dfv {
    Env::Env()
        : values(parseEnv()) {}

    std::string Env::operator[](const std::string &key) const {
        const auto it = values.find(key);
        if (it != values.end())
            return it->second;

        // Fallback to system env
        // ReSharper disable once CppDeprecatedEntity
        auto systemEnv = std::getenv(key.c_str());
        return systemEnv ? systemEnv : std::string{};
    }

    Env::EnvMap Env::parseEnv() {
        constexpr auto EnvPath = ".env";
        EnvMap values;

        // Try loading env from executale path and from current path
        const std::vector<std::filesystem::path> FilePaths = {
                EnvPath,
                getexepath().parent_path() / EnvPath,
        };

        // Add all variables from the env file to the map
        for (auto &path : FilePaths) {
            std::ifstream file(path);
            std::string line;

            while (std::getline(file, line)) {
                if (line[0] == '#')
                    continue;

                const auto pos = line.find('=');
                if (pos == std::string::npos)
                    continue;

                values.emplace(line.substr(0, pos), line.substr(pos + 1));
            }
        }

        return values;
    }
} // namespace dfv
