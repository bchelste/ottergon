#include "node_match.hpp"
#include <sstream>

namespace components::logical_plan {

    node_match_t::node_match_t(std::pmr::memory_resource *resource, const collection_full_name_t& collection)
        : node_t(resource, node_type::match_t, collection) {
    }

    hash_t node_match_t::hash_impl() const {
        return 0;
    }

    std::string node_match_t::to_string_impl() const {
        std::stringstream stream;
        stream << "$match: {";
        bool is_first = true;
        for (const auto& expr : expressions_) {
            if (is_first) {
                is_first = false;
            } else {
                stream << ", ";
            }
            stream << expr->to_string();
        }
        stream << "}";
        return stream.str();
    }


    node_ptr make_node_match(std::pmr::memory_resource *resource, const collection_full_name_t& collection, const ql::aggregate::match_t& match) {
        auto node = new node_match_t{resource, collection};
        if (match.query) {
            node->append_expression(match.query);
        }
        return node;
    }

}