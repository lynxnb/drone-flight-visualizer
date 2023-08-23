#include <iostream>
#include <glaze/glaze.hpp>
#include <cpr/cpr.h>
#include "fetcher.h"


namespace dfv::utils {
    using dfv::objects::location;
    std::vector<double> FetchElevation(const std::vector<point>& points) {
        // transform points to locations and write to json
        std::vector<location> locations(points.size());
        std::transform(points.begin(), points.end(), locations.begin(),
                       [](const point& point) -> location {
                           return {point.latitude, point.longitude};
                       });
        std::string buffer = glz::write_json(locations);

        // send request to open-elevation api
        cpr::Response r = cpr::Post(cpr::Url{"https://api.open-elevation.com/api/v1/lookup"},
                                    cpr::Body{"{\"locations\":" + buffer + "}"},
                                    cpr::Header{{"Content-Type", "application/json"}, {"Accept", "application/json"}});
        if (r.status_code != 200)
            throw std::runtime_error("Error in response while fetching elevation data with code " + std::to_string(r.status_code));
        std::string text = r.text;

        // parse response and return elevations
        auto s = glz::get_as_json<std::vector<point>,"/results">(text);
        if(s.has_value()) {
            std::vector<double> elevations(s.value().size());
            std::transform(s.value().begin(), s.value().end(), elevations.begin(),
                           [](const point& point) -> double {
                               return point.elevation;
                           });
            return elevations;
        } else {
            throw std::runtime_error("Error in response while fetching elevation data");
        }
    }
}

