#pragma once

#include <mutex>

namespace mem
{
	template<class>
	struct ScopedAccess;

	template<class T>
	struct MutexedObject
	{
		using ValueType = T;

		T object;
		std::mutex mutex;

		auto acquire() {
			return ScopedAccess(*this);
		}

		template<class... Args>
		MutexedObject(Args&&... args) : object(std::forward<Args>(args)...) {};
		~MutexedObject() = default;
	};

	template<class T>
	struct ScopedAccess
	{
		T& object;

		T::ValueType* operator->() {
			return &this->object.object;
		}

		ScopedAccess(T& object_) : object(object_) {
			this->object.mutex.lock();
		}

		~ScopedAccess() {
			this->object.mutex.unlock();
		}
	};
}
