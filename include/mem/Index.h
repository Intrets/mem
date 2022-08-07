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

template<class T, std::integral index_type_ = size_t>
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

	bool operator==(Index<T> const& other) {
		return this->i == other.i;
	}

	void set(index_type j) {
		this->i = j;
	}
};

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
