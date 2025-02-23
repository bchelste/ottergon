#pragma once

#include <vector>

#include <actor-zeta/actor-zeta.hpp>

#include <components/document/document_view.hpp>

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set_hook.hpp>

namespace components::cursor {

    using data_t = components::document::document_view_t;
    using data_ptr = const data_t*;
    using index_t = int32_t;
    constexpr index_t start_index = -1;

    class sub_cursor_t : public boost::intrusive::list_base_hook<> {
    public:
        sub_cursor_t(std::pmr::memory_resource* resource, actor_zeta::address_t collection);
        actor_zeta::address_t& address();
        std::size_t size() const;
        std::pmr::vector<data_t>& data();
        void append(data_t);

    private:
        actor_zeta::address_t collection_;
        std::pmr::vector<data_t> data_;
    };

    class cursor_t {
    public:
        cursor_t(std::pmr::memory_resource* resource);
        void push(sub_cursor_t* sub_cursor);
        std::size_t size() const;
        std::pmr::vector<std::unique_ptr<sub_cursor_t>>::iterator begin();
        std::pmr::vector<std::unique_ptr<sub_cursor_t>>::iterator end();
        bool has_next() const;
        data_ptr next();
        data_ptr get() const;
        data_ptr get(std::size_t index) const;
        void sort(std::function<bool(data_ptr, data_ptr)> sorter);

    private:
        std::size_t size_{};
        index_t current_index_{start_index};
        std::pmr::vector<std::unique_ptr<sub_cursor_t>> sub_cursor_;
        std::pmr::vector<data_ptr> sorted_;

        void create_list_by_sort();
        data_ptr get_sorted(std::size_t index) const;
        data_ptr get_unsorted(std::size_t index) const;
    };

} // namespace components::cursor
