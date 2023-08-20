#include <iostream>
#include <glaze/glaze.hpp>

bool init_vulkan();

struct my_struct
{
    int i = 287;
    double d = 3.14;
    std::string hello = "Hello World";
    std::array<uint64_t, 3> arr = { 1, 2, 3 };
};

template <>
struct glz::meta<my_struct> {
    using T = my_struct;
    static constexpr auto value = object(
            "i", &T::i,
            "d", &T::d,
            "hello", &T::hello,
            "arr", &T::arr
    );
};

int main() {
    my_struct s{};
    std::string buffer = glz::write_json(s);
    std::cout << buffer << std::endl;

    std::cout << "init_vulkan: " << init_vulkan() << std::endl;
    return 0;
}
