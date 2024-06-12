// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

#pragma once

#include <cstdint>
#include <concepts>

#ifdef LIB_SERIAL
#include <serial/Serializer.h>
#endif

using default_index_type = size_t;

template<class T, std::integral index_type_ = default_index_type>
struct Index
{
	using index_type = index_type_;
	index_type i;

	operator index_type() const {
		return i;
	}

	Index<T, index_type> operator++() {
		++i;
		return *this;
	}

	Index<T, index_type> operator++(int) {
		auto pre = *this;
		++i;
		return pre;
	}

	Index<T, index_type> operator--() {
		--i;
		return *this;
	}

	Index<T, index_type> operator--(int) {
		auto pre = *this;
		--i;
		return pre;
	}

	bool operator==(Index const& other) {
		return this->i == other.i;
	}

	bool operator!=(Index const& other) {
		return !(*this == other);
	}

	bool operator==(std::integral auto j) {
		return this->i == j;
	}

	bool operator!=(std::integral auto j) {
		return this->i != j;
	}

	void set(index_type j) {
		this->i = j;
	}

	Index() = default;
	~Index() = default;

	Index(std::integral auto j) : i(j) {}

	Index(Index const&) = default;
	Index(Index&&) = default;

	Index& operator=(Index const&) = default;
	Index& operator=(Index&&) = default;
};

template<class T, class S, class index_type = default_index_type>
struct IndexConverter;

namespace std
{
    template<class>
    struct hash;
}

template<class T>
struct std::hash<Index<T>>
{
	std::size_t operator()(Index<T> const& index) const noexcept {
		return std::hash<typename Index<T>::index_type>()(index.i);
	}
};

#ifdef LIB_SERIAL
template<class T>
struct serial::Serializable<Index<T>>
{
	static constexpr std::string_view typeName = "Index";

	ALL_DEF(Index<T>) {
		return serializer.runAll<Selector>(
			ALL(i)
			);
	}
};
#endif
