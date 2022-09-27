// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

#pragma once

#include <mutex>

#define DEFAULT_COPY(T) T(const T&) = default; T& operator=(const T&) = default;
#define NO_COPY(T) T(const T&) = delete; T& operator=(const T&) = delete;
#define DEFAULT_MOVE(T) T(T&&) = default; T& operator=(T&&) = default;
#define NO_MOVE(T) T(T&&) = delete; T& operator=(T&&) = delete;
#define DEFAULT_COPY_MOVE(T) DEFAULT_COPY(T) DEFAULT_MOVE(T)
#define NO_COPY_MOVE(T) NO_COPY(T) NO_MOVE(T)

namespace mem
{
	template<class>
	struct ScopedAccess;

	template<class T, class mutex_type_ = std::mutex>
	struct MutexedObject
	{
		using mutex_type = mutex_type_;

	private:
		friend struct ScopedAccess<MutexedObject<T, mutex_type>>;

		using ValueType = T;

		T object;
		mutex_type mutex;

	public:
		auto& cheat() {
			return this->object;
		}

		auto acquire() {
			return ScopedAccess<MutexedObject<T, mutex_type>>(*this);
		}

		void set(T&& object_) {
			auto access = this->acquire();

			*access = std::forward<T>(object_);
		}

		void set(T const& object_) {
			auto access = this->acquire();

			*access = object_;
		}

		T getCopy() {
			auto access = this->acquire();

			return *access;
		}

		template<class... Args>
		MutexedObject(Args&&... args) : object(std::forward<Args>(args)...) {
		};
		~MutexedObject() = default;

		NO_COPY(MutexedObject);
		DEFAULT_MOVE(MutexedObject);
	};

	template<class T>
	struct ScopedAccess
	{
		T& object;
		std::unique_lock<typename T::mutex_type> lock;

		typename T::ValueType* operator->() {
			return &this->object.object;
		}

		typename T::ValueType& operator*() {
			return this->object.object;
		}

		ScopedAccess(T& object_) : object(object_), lock(object.mutex) {
		}

		~ScopedAccess() = default;

		NO_COPY_MOVE(ScopedAccess);
	};
}
