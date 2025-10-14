#pragma once
#include <algorithm>
#include <type_traits>
#include <cstring>

template<typename T, typename U>
auto find_pod(T& collection, const U& value) {
    static_assert(std::is_pod<U>(), "Value must be a POD type!");
    auto first = collection.begin();
    auto last = collection.end();

    for (; first != last; first++) {
        if (memcmp(&(*first), &value, sizeof(value)) == 0) {
            return first;
        }
    }
    return last;
}

template<typename T, typename U>
auto find(T& collection, const U& value) {
    return std::find(collection.begin(), collection.end(), value);
}

template<typename T, typename U>
bool contains(const T& collection, const U& value) {
    for (const U& element : collection) {
        if (memcmp(&element, &value, sizeof(value)) == 0) {
            return true;
        }
    }
    /*
    if (!std::is_pod<U>()) {
        return find(collection, value) != collection.end();
    }
    */
    return false;
}

template<typename T, typename U>
void find_erase(T& collection, const U& value) {
    collection.erase(find(collection, value));
}

template<typename T, typename U>
void find_erase_pod(T& collection, const U& value) {
    collection.erase(find_pod(collection, value));
}

template<typename T, typename U>
auto concat(T& dest, const U& src) {
    dest.reserve(dest.size() + src.size());
    dest.insert(dest.end(), src.begin(), src.end());
}
