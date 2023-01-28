#pragma once
#include <unordered_map>

template <typename K, typename V> static V getOrDefault(const std::unordered_map<K, V>& m, const K& key, const V& def) {
  typename std::unordered_map<K, V>::const_iterator it = m.find(key);
  if (it == m.end()) {
    return def;
  } else {
    return it->second;
  }
}
