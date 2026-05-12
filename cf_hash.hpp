#pragma once

#include <cstddef>
#include <functional>
#include <new>
#include <utility>
#include <vector>
#include <memory>
#include <cstdint>
#include <stdexcept>

/// Collision-free hash table implementation
namespace cfhash {

    namespace impl {

        template <typename T, typename Comparator, typename Allocator, typename TIt = T>
        class fast_hash_table_impl {
        public:
            static constexpr std::uint32_t NO_NEXT = 0xffffffff;
            static constexpr std::uint32_t USED_MASK = 0x80000000;
            static constexpr std::uint32_t MAX_COUNT = 0x7fffffff;

            static size_t table_size_helper(size_t req) {
                size_t res = 16;
                if (req > USED_MASK) {
                    throw std::bad_alloc{};
                }

                while (res < req && res != USED_MASK) res <<= 1;

                return res;
            }

            struct item {
                std::uint32_t h = 0;
                std::uint32_t next;

                static constexpr std::size_t VALUE_SIZE = std::max<std::size_t>(sizeof(T), 1);
                alignas(alignof(T)) unsigned char buf[VALUE_SIZE];

                bool empty() const {
                    return !(h & USED_MASK);
                }

                void emplace(T && value) {
                    new (reinterpret_cast<T*>(buf)) T(std::move(value));
                }

                void emplace(const T & value) {
                    new (reinterpret_cast<T*>(buf)) T(value);
                }

                const TIt* get() const {
                    return reinterpret_cast<const TIt*>(buf);
                }

                TIt* get() {
                    return reinterpret_cast<TIt*>(buf);
                }

                T* get_mutable() {
                    return reinterpret_cast<T*>(buf);
                }

                void clear() {
                    if (!empty()) {
                        h = 0;
                        get_mutable()->~T();
                    }
                }

                ~item() {
                    clear();
                }

                item() = default;
                item(const item& other) {
                    emplace(other.get());
                    h = other.h;
                    next = other.next;
                }

                item(item&& other) noexcept {
                    emplace(std::move(*other.get()));
                    h = other.h;
                    next = other.next;
                }

                void swap(item& other) {
                    std::swap(*get_mutable(), *other.get_mutable());
                    std::swap(next, other.next);
                    std::swap(h, other.h);
                }
            };
            using inner_allocator  = typename std::allocator_traits<Allocator>::template rebind_alloc<item>;

            std::vector<item, inner_allocator> table;
            std::size_t count = 0;
            Comparator compare;

            fast_hash_table_impl(Comparator cmp, size_t initial_size, const Allocator& alloc)
                : table(table_size_helper(initial_size), alloc)
                , compare(cmp)
            {
                hash_mask = table.size() - 1;
            }

            std::pair<size_t, bool> insert(T && value, std::uint32_t h) {
                check_resize();
                h |= USED_MASK;

                std::size_t idx = h & hash_mask;

                if (table[idx].empty()) {
                    // Simple insert at empty position
                    table[idx].h = h;
                    table[idx].next = NO_NEXT;
                    table[idx].emplace(std::move(value));
                    ++count;
                    return { idx, true };
                }

                if ((table[idx].h & hash_mask) == idx) {
                    // We have elements in this bucket
                    while (table[idx].next != NO_NEXT) {
                        if (table[idx].h == h && compare(*table[idx].get(), value)) {
                            return { idx, false };
                        }
                        idx = table[idx].next;
                    }

                    // We didn't check it yet
                    if (table[idx].h == h && compare(*table[idx].get(), value)) {
                        return { idx, false };
                    }

                    // Find free element:
                    size_t idx2 = find_free_element(idx);
                    table[idx].next = idx2;
                    table[idx2].h = h;
                    table[idx2].emplace(std::move(value));
                    table[idx2].next = NO_NEXT;
                    ++count;
                    return { idx2, true };
                }

                // Here we have an element at this position, but we need to put head at this place. So, let's free the place:
                move_element_away(idx);

                table[idx].h = h;
                table[idx].next = NO_NEXT;
                table[idx].emplace(std::move(value));
                ++count;
                return { idx, true };
            }

            template <typename T2>
            size_t lookup(const T2& key, std::uint32_t h) const {
                size_t idx = h & hash_mask;
                h |= USED_MASK;

                if (table[idx].empty()) {
                    // Empty element: nothing to do
                    return NO_NEXT;
                }

                if ((table[idx].h & hash_mask) != idx) {
                    // Different bucket: no elements with this hash modulo size
                    return NO_NEXT;
                }

                while (idx != NO_NEXT) {
                    if (table[idx].h == h && compare(*table[idx].get(), key)) {
                        return idx;
                    }
                    idx = table[idx].next;
                }

                // OK, this bucket didn't contain the element
                return NO_NEXT;
            }

            void remove(size_t idx) {
                if (table[idx].empty()) {
                    return;
                }

                size_t head = table[idx].h & hash_mask;
                if (head == idx) { // We need to remove head
                    if (table[idx].next == NO_NEXT) {
                        table[idx].clear();
                        --count;
                        return;
                    }

                    size_t idx2 = table[idx].next;
                    table[idx].swap(table[idx2]);
                    table[idx2].clear();
                    --count;
                    return;
                }

                while (table[head].next != idx) {
                    head = table[head].next;
                }

                if (head == NO_NEXT) {
                    throw std::runtime_error("Corrupted hash table");
                }

                table[head].next = table[idx].next;
                table[idx].clear();
                --count;
            }

            void clear() {
                std::vector<item, inner_allocator> tmp(63);
                std::swap(tmp, table);
            }

            template <typename Item, typename Value>
            class iterator_impl {
                Item* ptr_;
                Item* end_;
                iterator_impl(Item* first, Item *last)
                    : ptr_(first)
                    , end_(last)
                {}

                friend class fast_hash_table_impl;
            public:
                iterator_impl()
                    : ptr_(nullptr)
                    , end_(nullptr)
                {
                }

                iterator_impl(const iterator_impl&) = default;
                iterator_impl(iterator_impl&&) noexcept = default;
                iterator_impl& operator = (const iterator_impl&) = default;
                iterator_impl& operator = (iterator_impl&&) noexcept = default;
                ~iterator_impl() = default;

                using value_type = Value;
                using reference = value_type&;
                using pointer = value_type*;
                using difference_type = std::ptrdiff_t;
                using iterator_category = std::forward_iterator_tag;

                reference operator*() {
                    return *ptr_->get();
                }

                pointer operator->() {
                    return ptr_->get();
                }

                bool operator == (const iterator_impl& other) const {
                    return ptr_ == other.ptr_;
                }

                bool operator != (const iterator_impl& other) const {
                    return ptr_ != other.ptr_;
                }

                iterator_impl& operator++() {
                    ++ptr_;
                    while (ptr_ != end_ && ptr_->empty()) {
                        ++ptr_;
                    }

                    return *this;
                }
            };

            using iterator = iterator_impl<item, TIt>;
            using const_iterator = iterator_impl<const item, const TIt>;

            iterator begin() {
                return iterator_at(first_index());
            }

            const_iterator cbegin() {
                return const_iterator_at(first_index());
            }

            iterator end() {
                return iterator_at(table.size());
            }

            const_iterator cend() {
                return const_iterator_at(table.size());
            }

            iterator iterator_at(size_t idx) {
                return iterator(table.data() + idx, table.data() + table.size());
            }

            const_iterator const_iterator_at(size_t idx) const {
                return const_iterator(table.data() + idx, table.data() + table.size());
            }

            const TIt& value_at(size_t idx) const {
                return *table[idx].get();
            }

            TIt& value_at(size_t idx) {
                return *table[idx].get();
            }

            void remove(iterator idx) {
                remove(idx.idx_);
            }

            void remove(const_iterator idx) {
                remove(idx.idx_);
            }
        private:
            // Implementation details
            size_t hash_mask = 0;

            void check_resize() {
                if (count == MAX_COUNT) {
                    throw std::bad_alloc();
                }

                if (table.size() == USED_MASK) {
                    return;
                }

                if (count + count / 8 >= table.size()) {
                    do_resize();
                }
            }

            void do_resize() {
                std::size_t new_size = table.size() * 2;

                std::vector<item, inner_allocator> tmp{new_size, table.get_allocator()};

                std::swap(tmp, table);
                count = 0;
                hash_mask = table.size() - 1;

                for (auto& item : tmp) {
                    if (!item.empty()) {
                        insert(std::move(*item.get()), item.h);
                    }
                }
            }

            size_t find_free_element(size_t idx) const {
                for (size_t i = 1; i < table.size(); ++i) {
                    ++idx;
                    if (idx >= table.size()) {
                        idx = 0;
                    }
                    if (table[idx].empty()) {
                        return idx;
                    }
                }

                throw std::bad_alloc();
            }

            void move_element_away(size_t idx) {
                size_t head = table[idx].h & hash_mask;
                if (head == idx) {
                    throw std::runtime_error("Internal error: can't move head element away");
                }

                while (table[head].next != idx) {
                    head = table[head].next;
                }

                size_t idx2 = find_free_element(idx);
                table[idx].swap(table[idx2]);
                table[head].next = idx2;
            }

            std::size_t first_index() const {
                size_t idx = 0;
                if (count == 0) {
                    return NO_NEXT;
                }
                const size_t size = table.size();
                while (idx < size && table[idx].empty()) {
                    ++idx;
                }

                if (idx >= size) {
                    return NO_NEXT;
                }

                return idx;
            }
        };

    } // namespace impl

    using std::hash;

    /** Default hash **/
    template <typename T, typename Hash = std::hash<T>>
    struct xorshift_hash_fixture {
        Hash hash_;

        explicit xorshift_hash_fixture(const Hash& h)
            : hash_(h)
        {
        }

        xorshift_hash_fixture()
            : hash_{}
        {
        }

        xorshift_hash_fixture(const xorshift_hash_fixture&) = default;
        xorshift_hash_fixture(xorshift_hash_fixture&&) noexcept = default;
        xorshift_hash_fixture& operator=(const xorshift_hash_fixture&) = default;
        xorshift_hash_fixture& operator=(xorshift_hash_fixture&&) noexcept = default;

        static std::uint32_t xorshift32(std::uint32_t x) {
            // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
            x ^= x << 13;
            x ^= x >> 17;
            x ^= x << 5;
            return x;
        }

        std::uint32_t operator()(const T& v) const {
            std::uint32_t h = hash_(v);
            return xorshift32(xorshift32(h));
        }
    };

    /** Interfaces **/
    template <typename T, typename Hash = xorshift_hash_fixture<T>, typename Comparator = std::equal_to<T>, typename Allocator = std::allocator<T>>
    class cf_hash_set {
        impl::fast_hash_table_impl<T, Comparator, Allocator> impl_;
        static constexpr std::uint32_t NO_NEXT = impl::fast_hash_table_impl<T, Comparator, Allocator>::NO_NEXT;
        Hash hash_;
    public:
        using size_type = std::size_t;
        using key_type = T;
        using value_type = T;
        using hasher = Hash;
        using key_equal = Comparator;
        using allocator_type = Allocator;
        using pointer = T*;
        using reference = T&;
        using const_pointer = const T*;
        using const_reference = const T&;
        static constexpr size_type default_initial_size = 63;

        cf_hash_set(size_type initial_size, const Hash& hash = {}, const key_equal& equal = {}, const allocator_type& allocator = {})
            : impl_(equal, initial_size, allocator)
            , hash_(hash)
        {
        }

        cf_hash_set(size_type initial_size, const Hash& hash, const allocator_type& allocator)
            : cf_hash_set(initial_size, hash, Comparator(), allocator)
        {
        }

        cf_hash_set(size_type initial_size, const allocator_type& allocator)
            : cf_hash_set(initial_size, Hash(), Comparator(), allocator)
        {
        }

        template <typename InputIter>
        cf_hash_set(InputIter first, InputIter last, size_type initial_size = default_initial_size, const Hash& hash = {}, const key_equal& equal = {}, const allocator_type& allocator = {})
            : cf_hash_set(initial_size, hash, equal, allocator)
        {
            while (first != last) {
                insert(*first++);
            }
        }

        cf_hash_set()
            : cf_hash_set(default_initial_size, Hash(), Comparator(), Allocator())
        {}

        cf_hash_set(const cf_hash_set& other) = default;
        cf_hash_set(cf_hash_set&& other) noexcept = default;
        cf_hash_set& operator=(const cf_hash_set& other) = default;
        cf_hash_set& operator=(cf_hash_set&& other) noexcept = default;
        ~cf_hash_set() = default;


        bool empty() const {
            return impl_.count == 0;
        }

        size_type size() const {
            return impl_.count;
        }

        size_type max_size() const {
            return 0xfffffffe;
        }

        void clear() {
            impl_.clear();
        }

        using iterator = typename impl::fast_hash_table_impl<T, Comparator, Allocator>::const_iterator;
        using const_iterator = typename impl::fast_hash_table_impl<T, Comparator, Allocator>::const_iterator;

        iterator begin() {
            return impl_.cbegin();
        }

        iterator end() {
            return impl_.cend();
        }

        const_iterator begin() const {
            return impl_.cbegin();
        }

        const_iterator end() const {
            return impl_.cend();
        }

        const_iterator cbegin() const {
            return impl_.cbegin();
        }

        const_iterator cend() const {
            return impl_.cend();
        }

        std::pair<iterator, bool> insert(const T& value) {
            std::uint32_t h = hash_(value);
            T tmp = value;
            auto res = impl_.insert(std::move(tmp), h);
            return {impl_.const_iterator_at(res.first), res.second};
        }

        std::pair<iterator, bool> insert(T&& value) {
            std::uint32_t h = hash_(value);
            auto res = impl_.insert(std::move(value), h);
            return {impl_.const_iterator_at(res.first), res.second};
        }

        std::pair<iterator, bool> insert_or_assign(const T& value) {
            std::uint32_t h = hash_(value);
            auto res = impl_.insert(std::move(value), h);
            if (!res.second) {
                *impl_.table[res.first].get() = value;
            }

            return {impl_.const_iterator_at(res.first), res.second};
        }

        void erase(iterator pos) {
            impl_.remove(pos);
        }

        void erase(const T& value) {
            std::uint32_t h = hash_(value);
            size_t idx = impl_.lookup(value, h);
            if (idx != NO_NEXT) {
                impl_.remove(idx);
            }
        }

        const_iterator find(const T& value) const {
            std::uint32_t h = hash_(value);
            std::size_t idx = impl_.lookup(value, h);
            return impl_.const_iterator_at(idx);
        }

        bool contains(const T& value) const {
            std::uint32_t h = hash_(value);
            std::size_t idx = impl_.lookup(value, h);
            return idx != NO_NEXT;
        }

        std::size_t count(const T& val) const {
            return contains(val)? 1: 0;
        }
    };

    /** Hash map **/
    template <typename Key, typename Val, typename Hash = xorshift_hash_fixture<Key>, typename Comparator = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<const Key, Val>>>
    class cf_hash_map {
        using item = std::pair<Key, Val>;

        struct value_compare {
            Comparator cmp;

            explicit value_compare(const Comparator& c = {})
                : cmp(c)
            {
            }

            bool operator () (const item& lval, const Key& rval) const {
                return cmp(lval.first, rval);
            }

            bool operator () (const item& lval, const item& rval) const {
                return cmp(lval.first, rval.first);
            }
        };

        using impl_type = impl::fast_hash_table_impl<item, value_compare, Allocator, std::pair<const Key, Val>>;
        impl_type impl_;
        static constexpr std::uint32_t NO_NEXT = impl::fast_hash_table_impl<item, Comparator, Allocator>::NO_NEXT;
        Hash hash_;
    public:
        using size_type = std::size_t;
        using key_type = Key;
        using mapped_type = Val;
        using value_type = std::pair<const Key, Val>;
        using pointer = value_type*;
        using reference = value_type&;
        using const_pointer = const value_type*;
        using const_reference = const value_type&;
        using difference_type = std::ptrdiff_t;
        using hasher = Hash;
        using key_equal = Comparator;
        using allocator_type = Allocator;

        static constexpr size_type default_initial_size = 63;

        cf_hash_map(size_type initial_size, const Hash& hash = {}, const key_equal& equal = {}, const allocator_type& allocator = {})
            : impl_(value_compare(equal), initial_size, allocator)
            , hash_(hash)
        {
        }

        cf_hash_map(size_type initial_size, const allocator_type& allocator)
            : cf_hash_map(initial_size, Hash{}, key_equal{}, allocator)
        {
        }

        cf_hash_map()
            : cf_hash_map(default_initial_size)
        {}

        template <typename InputIt>
        cf_hash_map(InputIt first, InputIt last, size_type initial_size = default_initial_size, const Hash& hash = {}, const key_equal& equal = {}, const allocator_type& allocator = {})
            : cf_hash_map(initial_size, hash, equal, allocator)
        {
            while (first != last) {
                insert(*first++);
            }
        }

        cf_hash_map(const cf_hash_map& other) = default;
        cf_hash_map(cf_hash_map&& other) noexcept = default;
        cf_hash_map& operator=(const cf_hash_map& other) = default;
        cf_hash_map& operator=(cf_hash_map&& other) noexcept = default;
        ~cf_hash_map() = default;

        bool empty() const {
            return impl_.count == 0;
        }

        size_type size() const {
            return impl_.count;
        }

        size_type max_size() const {
            return 0xfffffffe;
        }

        void clear() {
            impl_.clear();
        }

        using iterator = typename impl_type::iterator;
        using const_iterator = typename impl_type::const_iterator;

        iterator begin() {
            return impl_.begin();
        }

        iterator end() {
            return impl_.end();
        }

        const_iterator begin() const {
            return impl_.cbegin();
        }

        const_iterator end() const {
            return impl_.cend();
        }

        const_iterator cbegin() const {
            return impl_.cbegin();
        }

        const_iterator cend() const {
            return impl_.cend();
        }

        std::pair<iterator, bool> insert(const Key& value, const Val& val) {
            std::uint32_t h = hash_(value);
            item tmp{value, val};
            auto res = impl_.insert(std::move(tmp), h);
            return {impl_.iterator_at(res.first), res.second};
        }

        std::pair<iterator, bool> insert(value_type&& value) {
            std::uint32_t h = hash_(value);
            auto res = impl_.insert(std::move(value), h);
            return {impl_.iterator_at(res.first), res.second};
        }

        std::pair<iterator, bool> insert_or_assign(const value_type& value) {
            std::uint32_t h = hash_(value);
            auto res = impl_.insert(std::move(value), h);
            if (!res.second) {
                *impl_.table[res.first].get() = value;
            }

            return {impl_.iterator_at(res.first), res.second};
        }

        template <typename ...Args>
        std::pair<iterator, bool> emplace(Args&& ...args) {
            value_type val{std::forward<Args>(args)...};

            std::uint32_t h = hash_(val.first);
            auto res = impl_.insert(val, h);
            if (!res.second) {
                impl_.table[res.first].get()->second = std::move(val.second);
            }

            return std::pair<iterator, bool>(impl_.iterator_at(res.first), res.second);
        }

        void erase(iterator pos) {
            impl_.remove(pos);
        }

        void erase(const_iterator pos) {
            impl_.remove(pos);
        }

        void erase(const Key& value) {
            std::uint32_t h = hash_(value);
            size_t idx = impl_.lookup(value, h);
            if (idx != NO_NEXT) {
                impl_.remove(idx);
            }
        }

        iterator find(const Key& value) {
            std::uint32_t h = hash_(value);
            std::size_t idx = impl_.lookup(value, h);
            return impl_.iterator_at(idx);
        }

        const_iterator find(const Key& value) const {
            std::uint32_t h = hash_(value);
            std::size_t idx = impl_.lookup(value, h);
            return impl_.const_iterator_at(idx);
        }

        bool contains(const Key& value) const {
            std::uint32_t h = hash_(value);
            std::size_t idx = impl_.lookup(value, h);
            return idx != NO_NEXT;
        }

        std::size_t count(const Key& val) const {
            return contains(val)? 1: 0;
        }

        mapped_type& at(const key_type& key) {
            std::uint32_t h = hash_(key);
            std::size_t idx = impl_.lookup(key, h);
            if (idx == NO_NEXT) {
                throw std::out_of_range("Key not found");
            }

            return impl_.value_at(idx).second;
        }

        const mapped_type& at(const key_type& key) const {
            std::uint32_t h = hash_(key);
            std::size_t idx = impl_.lookup(key, h);
            if (idx == NO_NEXT) {
                throw std::out_of_range("Key not found");
            }

            return impl_.value_at(idx).second;
        }

        mapped_type& operator[] (const key_type& key) {
            std::uint32_t h = hash_(key);
            std::size_t idx = impl_.lookup(key, h);
            if (idx == NO_NEXT) {
                auto res = impl_.insert(std::make_pair(key, mapped_type{}), h);
                idx = res.first;
            }

            return impl_.value_at(idx).second;
        }

        mapped_type& operator[] (key_type&& key) {
            std::uint32_t h = hash_(key);
            std::size_t idx = impl_.lookup(key, h);
            if (idx == NO_NEXT) {
                auto res = impl_.insert(std::make_pair(key, mapped_type{}), h);
                idx = res.first;
            }

            return impl_.value_at(idx).second;
        }

        // Ignore it now
        void max_load_factor(float) {}

    };


} // namespace cfhash;
