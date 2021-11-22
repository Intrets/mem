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

#include <utility>
#include <cassert>

namespace impl
{
	template<class T>
	class Global
	{
	public:
		void provide(T* obj) const {
			destroy();
			object = obj;
		}

		void destroy() const {
			if (object != nullptr) {
				delete object;
				object = nullptr;
			}
		}

		T* operator->() const {
			assert(object != nullptr);
			return object;
		}

		T& operator*() const {
			assert(object != nullptr);
			return *object;
		}

		template<class ...Args>
		void init(Args&& ...args) const {
			provide(new T(std::forward<Args>(args)...));
		}

	private:
		static inline T* object = nullptr;
	};
}

namespace mem
{
	template<class T>
	constexpr auto Global = impl::Global<T>{};
}

using mem::Global;
