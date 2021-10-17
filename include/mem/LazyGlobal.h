#pragma once

namespace impl
{
	template<class T>
	struct LazyGlobal
	{
		T* operator*() const {
			static T global{};
			return &global;
		}

		T* operator->() const {
			return LazyGlobal<T>::operator*();
		}
	};
}

template<class T>
constexpr impl::LazyGlobal<T> LazyGlobal = impl::LazyGlobal<T>{};
