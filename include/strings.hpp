#include <string>
#include <charconv>

template <typename T>
inline std::optional<T> parse_num(const std::string &str)
{
    T value{};
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec != std::errc{})
        return std::nullopt;
    return value;
}

inline std::vector<std::string> split(const std::string &str)
{
    return str | std::views::split(',') | std::ranges::to<std::vector<std::string>>();
}

inline std::string parse_chars(const std::string &str)
{
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char c)
                           { return std::isdigit(c); });

    if (it != str.end())
    {
        std::size_t i = std::distance(str.begin(), it);
        return str.substr(0, i);
    }
    return "";
}