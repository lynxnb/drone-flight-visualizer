#include <vector>
#include "../objects/objects.cpp"

using point = dfv::objects::point;

namespace dfv::utils {
    class Fetcher {
    public:
        static std::vector<point> FetchElevation(const std::vector<point>& points);
    private:

    };
}
