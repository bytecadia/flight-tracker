#include <string>

inline std::optional<int> parse_int(const std::string &str)
{
    return !str.empty() ? std::optional<int>(std::stoi(str)) : std::nullopt;
}

inline std::optional<double> parse_dbl(const std::string &str)
{
    return !str.empty() ? std::optional<double>(std::stod(str)) : std::nullopt;
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
        return str.substr(0, i + 1);
    }
    return "";
}