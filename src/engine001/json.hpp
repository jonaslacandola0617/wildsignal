#pragma once
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace ws {
// Bounded authoring-file reader, not a general asset/save system.
struct Json {
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json, std::less<>>;
    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;
    const Json& at(std::string_view key) const;
    const Array& array() const;
    const std::string& string() const;
    double number() const;
};
Json parseJson(std::string_view text);
std::string quoteJson(std::string_view text);
}
