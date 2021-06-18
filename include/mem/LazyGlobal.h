#pragma once

namespace impl
{
	template<class T>
	struct LazyGlobal
	{
		T* operator->() const {
			static T global{};
			return &global;
		}
	};
}

template<class T>
constexpr impl::LazyGlobal<T> LazyGlobal = impl::LazyGlobal<T>{};