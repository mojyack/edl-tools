#include <stack>

#include "xml.hpp"

namespace {
template <class T, class... Args>
static auto contains(const T value, const T key, const Args... args) -> bool {
    if(value == key) {
        return true;
    }
    if constexpr(sizeof...(Args) > 0) {
        return contains(value, args...);
    }
    return false;
};
} // namespace

namespace xml {
struct StringReader {
    size_t           cursor;
    std::string_view str;

    auto peek() const -> char {
        if(cursor >= str.size()) {
            throw std::runtime_error("unexpected eof");
        }
        return str[cursor];
    }

    auto read() -> char {
        const auto c = peek();
        cursor += 1;
        return c;
    }

    auto is_eof() const -> bool {
        return cursor >= str.size();
    }

    template <class... Args>
    auto read_until(const Args... args) -> std::string_view {
        auto begin = cursor;
    loop:
        const auto c = read();
        if(contains(c, args...)) {
            cursor -= 1;
            return str.substr(begin, cursor - begin);
        }
        goto loop;
    }

    auto skip_while(const char key) -> void {
        while(peek() == key) {
            cursor += 1;
        }
    }
};

struct ParseElementNodeResult {
    Node node;
    bool leaf;
};

auto parse_element_node(StringReader& reader) -> Result<ParseElementNodeResult> {
    if(reader.read() != '<') {
        return Error("not a xml");
    }

    auto node = Node();
    {
        auto begin = reader.cursor;
        while(true) {
            const auto c = reader.read();
            if(c == ' ' || c == '>') {
                node.name = reader.str.substr(begin, reader.cursor - begin - 1);
                if(c == '>') {
                    auto leaf = false;
                    if(node.name.back() == '/') {
                        node.name.remove_suffix(1);
                        leaf = true;
                    }
                    return ParseElementNodeResult{node, leaf};
                }
                break;
            }
        }
    }
    while(true) {
        reader.skip_while(' ');
        const auto key = reader.read_until('=', '>');
        reader.read(); // skip '=' or '>'
        if(key == "" || key == "/") {
            return ParseElementNodeResult{node, key == "/"};
        }
        const auto dlm   = reader.read();
        const auto value = reader.read_until(dlm);
        reader.read(); // skip dlm
        node.attrs.push_back({key, value});
    }

    return Error("not closed");
}

auto parse_nodes(const std::string_view str) -> Result<Node> {
    auto root       = Node();
    auto node_stack = std::stack<Node*>();
    auto reader     = StringReader{.str = str};

    node_stack.push(&root);

    while(!reader.is_eof()) {
        if(reader.peek() == '<') {
            auto pnr_r = parse_element_node(reader);
            if(!pnr_r) {
                return pnr_r.as_error();
            }
            auto& [node, leaf] = pnr_r.as_value();

            if(node.name.front() == '/') {
                node_stack.pop();
                continue;
            }
            if(leaf) {
                node_stack.top()->children.emplace_back(node);
                continue;
            }

            auto& new_node = node_stack.top()->children.emplace_back(node);
            node_stack.push(&new_node);
        } else {
            const auto data = reader.read_until('<');
            if(data != "\n") {
                node_stack.top()->data = data;
            }
        }
    }
    return root.children[0];
}

auto parse(const std::string_view str) -> Result<Node> {
    try {
        return parse_nodes(str);
    } catch(const std::runtime_error& e) {
        return Error(e.what());
    }
}
} // namespace xml
