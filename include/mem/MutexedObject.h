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

#include <mutex>

#include <misc/Misc.h>

namespace mem
{
	template<class>
	struct ScopedAccess;

	template<class T>
	struct MutexedObject
	{
	private:
		friend struct ScopedAccess<MutexedObject<T>>;

		using ValueType = T;

		T object;
		std::mutex mutex;

	public:
		auto acquire() {
			return ScopedAccess(*this);
		}

		template<class... Args>
		MutexedObject(Args&&... args) : object(std::forward<Args>(args)...) {};
		~MutexedObject() = default;

		NO_COPY(MutexedObject);
		DEFAULT_MOVE(MutexedObject);
	};

	template<class T>
	struct ScopedAccess
	{
		T& object;

		typename T::ValueType* operator->() {
			return &this->object.object;
		}

		typename T::ValueType& operator*() {
			return this->object.object;
		}

		ScopedAccess(T& object_) : object(object_) {
			this->object.mutex.lock();
		}

		~ScopedAccess() {
			this->object.mutex.unlock();
		}
	};
}
