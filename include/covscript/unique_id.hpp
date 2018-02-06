#include <random>
#include <string>

namespace cs_impl {

namespace unique_id {
    // https://stackoverflow.com/a/24586587
    std::string random_string(std::string::size_type length)
    {
        static auto& chrs =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        thread_local static std::mt19937 rg{std::random_device{}()};
        thread_local static std::uniform_int_distribution<std::string::size_type> pick(0, sizeof(chrs) - 2);

        std::string s;

        s.reserve(length);

        while(length--)
            s += chrs[pick(rg)];

        return s;
    }

} // namespace unique_id
} // namespace cs_impl

