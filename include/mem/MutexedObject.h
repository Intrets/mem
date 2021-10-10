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

		T::ValueType* operator->() {
			return &this->object.object;
		}

		T::ValueType& operator*() {
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
