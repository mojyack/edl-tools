#pragma once
#include <optional>
#include <string_view>
#include <vector>

#include "../util/error.hpp"

namespace xml {
struct Attribute {
    std::string_view key;
    std::string_view value;
};

struct AttributeQuery {
    std::string_view  key;
    std::string_view& value;
};

struct Node {
    std::string_view       name;
    std::string_view       data;
    std::vector<Attribute> attrs;
    std::vector<Node>      children;

    auto find_attr(std::string_view key) const -> std::optional<std::string_view>;
    auto is_attr_equal(std::string_view key, std::string_view value) const -> bool;
    auto get_attrs(AttributeQuery* queries, size_t len) const -> bool;

    template <size_t N>
    auto get_attrs(AttributeQuery (&&queries)[N]) const -> bool {
        return get_attrs(queries, N);
    }

    template <class Name, class... Names>
    auto find_first_child(const Name& name, const Names&... names) const -> const Node* {
        for(const auto& c : children) {
            if(c.name != name) {
                continue;
            }
            if constexpr(sizeof...(Names) > 0) {
                return c.find_first_child(names...);
            } else {
                return &c;
            }
        }
        return nullptr;
    }

    auto clone() const -> Node {
        return *this;
    }

    auto set_data(const std::string_view data) -> Node&& {
        this->data = data;
        return std::move(*this);
    }

    template <size_t N>
    auto append_attrs(Attribute (&&attrs)[N]) -> Node&& {
        this->attrs.insert(this->attrs.end(), std::begin(attrs), std::end(attrs));
        return std::move(*this);
    }

    template <size_t N>
    auto append_children(Node (&&nodes)[N]) -> Node&& {
        this->children.insert(this->children.end(), std::move_iterator(std::begin(nodes)), std::move_iterator(std::end(nodes)));
        return std::move(*this);
    }

    auto operator==(const Node& o) const -> bool;
};

// parser.cpp
auto parse(const std::string_view str) -> Result<Node>;

// deparser.cpp
auto deparse(const Node& node) -> std::string;

// test.cpp
auto dump_node(const Node& node, std::string_view prefix = "", bool print_empty_fields = false) -> void;
} // namespace xml
