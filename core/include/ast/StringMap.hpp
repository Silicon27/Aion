//
// Created by David Yang on 2026-02-08.
//

#ifndef AION_STRING_MAP_HPP
#define AION_STRING_MAP_HPP

#include <algorithm>
#include <cstddef>
#include <limits>
#include <new>
#include <string_view>
#include <type_traits>
#include <utility>

#define XXH_INLINE_ALL
#include <support/xxhash.h>

namespace aion::ast {
	class ASTContext;

	template<typename T>
	constexpr T next_power_of_two(T x) {
		static_assert(std::is_unsigned_v<T>, "T must be unsigned");

		if (x <= 1) return 1;

		--x; // critical to handle exact powers of two

		for (std::size_t i = 1; i < std::numeric_limits<T>::digits; i <<= 1) {
			x |= x >> i;
		}

		return x + 1;
	}

	/// Open addressing
	template<typename Value, typename Context = ASTContext>
	class StringMap {
		struct Slot {
			std::size_t hash;
			std::string_view key;
			Value value;
			bool occupied;

			Slot() : hash(0), key(), value(), occupied(false) {
			}

			Slot(std::string_view k, Value v, std::size_t h, bool o)
				: hash(h), key(k), value(v), occupied(o) {
			}
		};

		std::size_t capacity; // num of allocated slots
		std::size_t size; // num of occupied slots
		std::size_t bytes_used; // num of bytes used
		Context &ctx;
		Slot *slots;
		double max_load_factor = 0.75f;

	public:
		using value_type = Value;
		using allocator_type = Context;
		using key_type = std::string_view;
		using mapped_type = Value;
		using reference = Value &;
		using const_reference = const Value &;
		using pointer = Value *;
		using const_pointer = const Value *;
		using size_type = std::size_t;

		[[nodiscard]] std::size_t get_size() const { return size; }
		[[nodiscard]] std::size_t get_bytes_used() const { return bytes_used; }
		[[nodiscard]] std::size_t get_capacity() const { return capacity; }
		[[nodiscard]] bool empty() const { return size == 0; }

		[[nodiscard]] float load_factor() const {
			return capacity == 0 ? 0.0f : static_cast<float>(size) / static_cast<float>(capacity);
		}

		[[nodiscard]] float get_max_load_factor() const { return max_load_factor; }
		void set_max_load_factor(const float factor) { max_load_factor = factor; }

		explicit StringMap(Context &ctx, std::size_t initial_capacity = 64) : size(0), bytes_used(0), ctx(ctx) {
			capacity = next_power_of_two(std::max<std::size_t>(initial_capacity, 4));
			std::size_t bytes = capacity * sizeof(Slot);
			slots = static_cast<Slot *>(ctx.allocate(bytes, alignof(Slot)));
			for (std::size_t i = 0; i < capacity; ++i) {
				new(&slots[i]) Slot();
			}
		}

		void rehash(size_type new_cap) {
			new_cap = next_power_of_two(new_cap);
			if (new_cap <= capacity) return;

			Slot *old_slots = slots;
			std::size_t old_cap = capacity;

			std::size_t bytes = new_cap * sizeof(Slot);
			slots = static_cast<Slot *>(ctx.allocate(bytes, alignof(Slot)));
			capacity = new_cap;

			for (std::size_t i = 0; i < capacity; ++i) {
				new(&slots[i]) Slot();
			}

			size = 0;
			bytes_used = 0;

			for (std::size_t i = 0; i < old_cap; ++i) {
				if (old_slots[i].occupied) {
					insert_with_hash(old_slots[i].key, old_slots[i].value, old_slots[i].hash);
				}
			}
		}

		bool is_full() const {
			if (capacity == 0) return true;
			return (static_cast<double>(size) + 1) > static_cast<double>(capacity) * max_load_factor;
		}

		void insert(std::pair<const std::string_view, Value> const &value) {
			const std::size_t h = hash_key(value.first);
			if (Value *v = find_with_hash(value.first, h)) {
				*v = value.second;
				return;
			}
			const char *storage = ctx.allocate_string(value.first);
			insert_with_hash(std::string_view(storage, value.first.size()), value.second, h);
		}

		void insert(std::string_view key, Value value) {
			const std::size_t h = hash_key(key);
			if (Value *v = find_with_hash(key, h)) {
				*v = value;
				return;
			}
			const char *storage = ctx.allocate_string(key);
			insert_with_hash(std::string_view(storage, key.size()), value, h);
		}

		template<typename... Args>
		void emplace(key_type k, Args &&... args)
			requires std::is_constructible_v<Value, Args...> {
			const std::size_t h = hash_key(k);
			if (Value *v = find_with_hash(k, h)) {
				*v = Value(std::forward<Args>(args)...);
				return;
			}
			char *storage = ctx.allocate_string(k);
			insert_with_hash(std::string_view(storage, k.size()), Value(std::forward<Args>(args)...), h);
		}

		template<typename... Args>
		mapped_type &emplace_or_get(key_type k, Args &&... args)
			requires std::is_constructible_v<Value, Args...> {
			const std::size_t h = hash_key(k);
			if (Value *v = find_with_hash(k, h)) {
				return *v;
			}
			char *storage = ctx.allocate_string(k);
			return *insert_with_hash(std::string_view(storage, k.size()), Value(std::forward<Args>(args)...), h);
		}

		mapped_type &operator[](std::string_view key)
			requires std::is_default_constructible_v<mapped_type> {
			const std::size_t h = hash_key(key);
			if (Value *v = find_with_hash(key, h)) {
				return *v;
			}
			char *storage = ctx.allocate_string(key);
			return *insert_with_hash(std::string_view(storage, key.size()), Value(), h);
		}

		const Value *find(std::string_view key) const {
			if (capacity == 0) return nullptr;
			const std::size_t h = hash_key(key);
			return find_with_hash(key, h);
		}

		Value *find(std::string_view key) {
			if (capacity == 0) return nullptr;
			const std::size_t h = hash_key(key);
			return find_with_hash(key, h);
		}

		const Value *at(std::string_view key) const {
			if (capacity == 0) return nullptr;
			const std::size_t h = hash_key(key);
			return find_with_hash(key, h);
		}

		Value *at(std::string_view key) {
			if (capacity == 0) return nullptr;
			const std::size_t h = hash_key(key);
			return find_with_hash(key, h);
		}

	private:
		static size_t hash_key(std::string_view s) { return XXH3_64bits(s.data(), s.size()); }

		Value *find_with_hash(std::string_view key, std::size_t h) const {
			if (capacity == 0) return nullptr;
			std::size_t idx = h & (capacity - 1);
			std::size_t start_idx = idx;
			while (slots[idx].occupied) {
				if (slots[idx].hash == h && slots[idx].key == key) {
					return &slots[idx].value;
				}
				idx = (idx + 1) & (capacity - 1);
				if (idx == start_idx) break;
			}
			return nullptr;
		}

		Value *insert_with_hash(std::string_view key, Value value, std::size_t h) {
			if (is_full()) rehash(capacity * 2);

			std::size_t idx = h & (capacity - 1);
			while (slots[idx].occupied) {
				if (slots[idx].hash == h && slots[idx].key == key) {
					slots[idx].value = value;
					return &slots[idx].value;
				}
				idx = (idx + 1) & (capacity - 1);
			}
			slots[idx] = Slot(key, value, h, true);
			++size;
			bytes_used += key.size();
			return &slots[idx].value;
		}
	};
} // namespace aion::ast

#endif //AION_STRING_MAP_HPP

