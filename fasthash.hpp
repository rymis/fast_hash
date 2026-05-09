#pragma once

#include <vector>
#include <cstdint>
#include <stdexcept>

namespace fhash {

    namespace detail {

        template <typename Key, typename Value>
        struct HashTable {
            using key_type = Key;
            using mapped_type = Value;
            using value_type = std::pair<const key_type, mapped_type>;

            struct Item {
                std::pair<const Key, Value> value;
                uint32_t hash = 0;

                Item() = default;
                Item(const Key& k, Value&& v, uint32_t h)
                    : value(k, std::forward<Value>(v))
                    , hash(h)
                {
                }
            };

            struct Node {
                std::vector<Item> items;
                std::vector<Node> children;

                Item* find(const Key& key, uint32_t h) {
                    if (children.empty()) {
                        for (auto& item : items) {
                            if (item.hash == h && item.value.first == key) {
                                return &item;
                            }
                        }

                        return nullptr;
                    }

                    return children[h & 0x0f].find(key, h >> 4);
                }

                const Item* find(const Key& key, uint32_t h) const {
                    if (children.empty()) {
                        for (const auto& item : items) {
                            if (item.hash == h && item.value.first == key) {
                                return &item;
                            }
                        }

                        return nullptr;
                    }

                    return children[h & 0x0f].find(key, h >> 4);
                }

                Item* insert(const Key& key, uint32_t h, bool replace, int level, mapped_type&& val) {
                    if (children.empty()) {
                        for (auto& item : items) {
                            if (item.hash == h && item.value.first == key) {
                                if (replace) {
                                    item.value.second = std::forward<mapped_type>(val);
                                }

                                return &item;
                            }
                        }

                        if (level == 7) {
                            // We can't increase level
                            items.emplace_back(key, std::forward<mapped_type>(val), h);

                            return &items.back();
                        }

                        children.resize(16);
                        for (auto& item: items) {
                            children[(item.hash >> (level * 4)) & 0x0f].insert(item.value.first, item.hash, replace, level + 1, std::move(item.value.second));
                        }
                        items.clear();

                        return children[(h >> (level * 4)) & 0x0f].insert(key, h, replace, level + 1, std::forward<mapped_type>(val));
                    } else {
                        return children[(h >> (level * 4)) & 0x0f].insert(key, h, replace, level + 1, std::forward<mapped_type>(val));
                    }
                }
            };

            struct Iter {
                Node* root = nullptr;
                int8_t path[8];
                uint32_t idx = 0;

                bool next();
                bool prev();

                Item& get();
            };

            Node root;

            const Item* find(const Key& key, uint32_t h) const {
                return root.find(key, h);
            }

            Item* insert(const Key& key, uint32_t h, mapped_type&& val) {
                return root.insert(key, h, false, 7, std::forward<mapped_type>(val)); 
            }

            Item* insert_or_replace(const Key& key, uint32_t h, mapped_type&& val) {
                return root.insert(key, h, true, 7, std::forward<mapped_type>(val)); 
            }
        };

    } // namespace detail

    template <class Key, class Value, class Hash = std::hash<Key>>
    class fasthash {
        detail::HashTable<Key, Value> table_;
        Hash hash_;

    public:
        fasthash() = default;
        fasthash(const fasthash&) = default;
        fasthash(fasthash&&) noexcept = default;
        ~fasthash() = default;

        using key_type = Key;
        using mapped_type = Value;
        using value_type = std::pair<const key_type, mapped_type>;

        mapped_type& operator[](const key_type& key) {
            uint32_t h = hash_(key);
            auto res = table_.insert(key, h, mapped_type());
            return res->value.second;
        }

        mapped_type& at(const key_type& key) {
            uint32_t h = hash_(key);
            auto res = table_.find(key, h);
            if (!res) {
                throw std::range_error("Element is not found");
            }

            return res->value.second;
        }

        const mapped_type& at(const key_type& key) const {
            uint32_t h = hash_(key);
            auto res = table_.find(key, h);
            if (!res) {
                throw std::range_error("Element is not found");
            }

            return res->value.second;
        }
    };

} // namespace fhash
