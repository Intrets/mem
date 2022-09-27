// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

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

		template<class ...Args>
		void tryInit(Args&& ...args) const {
			if (this->object == nullptr) {
				provide(new T(std::forward<Args>(args)...));
			}
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
