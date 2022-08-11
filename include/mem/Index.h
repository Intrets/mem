// mem - A C++ library for managing objects
// Copyright (C) 2021  Intrets
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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

	template<class S>
	Index(S const& s);
};

template<class T, class S, class index_type = default_index_type>
struct IndexConverter;

template<class T>
struct std::hash<Index<T>>
{
	std::size_t operator()(Index<T> const& index) const noexcept {
		return std::hash<Index<T>::index_type>()(index.i);
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

template<class T, std::integral index_type_>
template<class S>
inline Index<T, index_type_>::Index(S const& s) : Index<T, index_type_>(IndexConverter<T, S, index_type_>::run(s)) {
};

