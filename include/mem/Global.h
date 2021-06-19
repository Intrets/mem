#pragma once

#include <utility>

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

template<class T>
constexpr auto Global = impl::Global<T>{};
