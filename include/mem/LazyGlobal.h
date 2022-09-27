// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

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
			return LazyGlobal<T, phantom>::operator*();
		}

		T& get() const {
			return *LazyGlobal<T, phantom>::operator*();
		}
	};
}

template<class T, class phantom = void>
constexpr impl::LazyGlobal<T, phantom> LazyGlobal = impl::LazyGlobal<T, phantom>{};
