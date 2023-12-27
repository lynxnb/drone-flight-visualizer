#pragma once

#include <string>
#include <unordered_map>

namespace dfv {
    /**
     * @brief A class for parsing env files into a map and retrieving env variables.
     * When retrieving an env variable, variables from .env files are prioritized over system env variables.
     * Order of .env files is unspecified.
     */
    class Env {
      public:
        /**
         * @brief Returns the env variable with the given key.
         * @return The env variable value, or an empty string if it doesn't exist.
         */
        std::string operator[](const std::string &key) const;

        Env();

      private:
        using EnvMap = std::unordered_map<std::string, std::string>;

        /**
         * @brief Parses env files into a map.
         * Files are searched in the following directories:
         * - The current working directory
         * - The directory of the executable
         * @return A map of the env file.
         */
        static EnvMap parseEnv();

        /**
         * @brief The map of env variables.
         */
        const EnvMap values;
    };

    inline Env env{}; //!< The global env instance
} // namespace dfv
