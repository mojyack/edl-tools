#include "xml.hpp"

namespace xml {
auto dump_node(const Node& node, const std::string_view prefix, const bool print_empty_fields) -> void {
    const auto prefix2 = std::string(prefix) + "  ";
    const auto prefix4 = std::string(prefix) + "    ";

    print(prefix, ".name = \"", node.name, "\",");
    print(prefix, ".data = \"", node.data, "\",");
    if(!(node.attrs.empty() && !print_empty_fields)) {
        print(prefix, ".attrs = {");
        for(const auto& a : node.attrs) {
            print(prefix2, "{\"", a.key, "\", \"", a.value, "\"},");
        }
        print(prefix, "},");
    }
    if(!(node.children.empty() && !print_empty_fields)) {
        print(prefix, ".children = {");
        for(const auto& c : node.children) {
            print(prefix2, "Node{");
            dump_node(c, prefix4);
            print(prefix2, "},");
        }
        print(prefix, "},");
    }
}

auto test() -> void {
    struct Case {
        std::string str;
        Node        node;
    };
    static const auto cases = std::array{
        Case{
            "<data>\n<response value=\"ACK\" rawmode=\"true\" />\n</data>",
            Node{
                .name = "data",
            }
                .append_children({
                    Node{
                        .name = "response",
                    }
                        .append_attrs({
                            {"value", "ACK"},
                            {"rawmode", "true"},
                        }),
                }),
        },
    };
    for(const auto& c : cases) {
        print("test str: ", c.str);
        auto n_r = parse(c.str);
        if(!n_r) {
            print("fail: ", n_r.as_error());
            continue;
        }
        const auto& n = n_r.as_value();
        print("deparsed: ", deparse(n));
        if(n != c.node) {
            print("result did not match");
            dump_node(n);
        } else {
            print("pass");
        }
    }
}
} // namespace xml
