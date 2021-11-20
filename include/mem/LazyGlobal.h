#pragma once

namespace impl
{
	template<class T, class phantom = void>
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

template<class T, class phantom = void>
constexpr impl::LazyGlobal<T, phantom> LazyGlobal = impl::LazyGlobal<T, phantom>{};
