#pragma once

#include <cstdint>
#include <string_view>

namespace dfv {
    using namespace std::string_view_literals;

    namespace window_config {
        constexpr const char *WindowTitle = "Drone Flight Visualizer";
        constexpr uint32_t WindowWidth = 1280;
        constexpr uint32_t WindowHeight = 720;
    } // namespace window_config
} // namespace dfv
