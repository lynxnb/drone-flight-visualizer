#include <iostream>
#include <glaze/glaze.hpp>
#include <cpr/cpr.h>
#include "fetcher.h"

namespace dfv::utils {
    std::vector<point> FetchElevation(const std::vector<point>& points) {
        std::string locations;
        for (auto &point: points) {
            locations += std::to_string(point.latitude) + "," + std::to_string(point.longitude) + "|";
        }
        cpr::Response r = cpr::Get(cpr::Url{"https://api.open-elevation.com/api/v1/lookup"},
                                   cpr::Parameters{{"locations", locations}});
        if (r.status_code != 200)
            throw std::runtime_error("Error in response while fetching elevation data.");
        std::string text = r.text;

        std::cout << text << std::endl;
        auto s = glz::read_json<std::vector<point>>(text);
        if (!s) return {};
        return s.value();
    }
}