#include <dictionary.hpp>

template <typename KV, typename KeyT, typename ValueT>
KV add_all(KV kv, KeyT key, ValueT value) {
	kv[key] = value;
	return kv;
}

template <typename KV, typename KeyT, typename ValueT, typename... Args>
KV add_all(KV kv, KeyT key, ValueT value, Args... args) {
	kv[key] = value;
	return add_all(kv, args...);
}

template <typename KV>
KV add_all(KV kv) {
	return kv;
}

template <typename... Args>
godot::Dictionary makeDict(Args... args) {
  return add_all(godot::Dictionary(), args...);
}
