#include "xml.hpp"

namespace xml {
auto deparse(std::string& str, const Node& node) -> void {
    str += "<";
    str += node.name;
    for(const auto& a : node.attrs) {
        str += " ";
        str += a.key;
        str += "=\"";
        str += a.value;
        str += '\"';
    }

    const auto leaf = node.data.empty() && node.children.empty();
    if(leaf) {
        str += "/>";
        return;
    }
    str += ">";
    if(!node.data.empty()) {
        str += node.data;
    }
    for(const auto& c : node.children) {
        deparse(str, c);
    }
    str += "</";
    str += node.name;
    str += ">";
}

auto deparse(const Node& node) -> std::string {
    auto str = std::string();
    deparse(str, node);
    return str;
}
} // namespace xml
