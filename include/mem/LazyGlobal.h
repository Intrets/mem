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
