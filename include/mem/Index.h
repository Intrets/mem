#pragma once

#include <cstdint>
#include <concepts>

#include <serial/Serializer.h>

template<class T, std::integral index_type = size_t>
struct Index
{
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

	void set(index_type j) {
		this->i = j;
	}
};

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
