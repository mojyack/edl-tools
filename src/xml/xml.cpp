#include "xml.hpp"

namespace xml {
auto Node::get_attrs(AttributeQuery* const queries, const size_t len) const -> bool {
    auto found = 0u;
    for(auto i = 0u; i < len; i += 1) {
        auto& q = queries[i];
        for(const auto& a : attrs) {
            if(a.key == q.key) {
                q.value = a.value;
                found += 1;
                break;
            }
        }
    }
    return found == len;
}

auto Node::find_attr(const std::string_view key) const -> std::optional<std::string_view> {
    for(const auto& a : attrs) {
        if(a.key == key) {
            return a.value;
        }
    }
    return std::nullopt;
}

auto Node::is_attr_equal(std::string_view key, std::string_view value) const -> bool {
    const auto attr_o = find_attr(key);
    if(!attr_o.has_value()) {
        return false;
    }
    return (*attr_o) == value;
}

auto Node::operator==(const Node& o) const -> bool {
    if(name != o.name || data != o.data || attrs.size() != o.attrs.size() || children.size() != o.children.size()) {
        print(name, " ", o.name);
        print(data, " ", o.data);
        print(attrs.size(), " ", o.attrs.size());
        print(children.size(), " ", o.children.size());
        return false;
    }
    for(auto i = 0u; i < attrs.size(); i += 1) {
        if(attrs[i].key != o.attrs[i].key || attrs[i].value != o.attrs[i].value) {
            return false;
        }
    }
    for(auto i = 0u; i < children.size(); i += 1) {
        if(children[i] != o.children[i]) {
            return false;
        }
    }
    return true;
}

auto deparse(const Node& node) -> std::string;
} // namespace xml
