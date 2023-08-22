
#include "fetcher.h"
#include <cpr/cpr.h>
#include <iostream>


/*template<>
struct glz::meta<point> {
    using T = point;
    static constexpr auto value = object(
            "latitude", &T::latitude,
            "longitude", &T::longitude,
            "elevation", &T::elevation
    );
};*/

namespace dfv::utils {
    std::vector<point> Fetcher::FetchElevation(const std::vector<point>& points) {
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
     /*   auto s = glz::read_json<std::vector<point>>(text);
        if (!s) return {};
        return s.value();*/
        return {};
    }
}