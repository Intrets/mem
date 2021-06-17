#pragma once

#include <utility>

template<class T>
class Global
{
public:
	static void provide(T* obj) {
		destroy();
		object = obj;
	}

	static void destroy() {
		if (object != nullptr) {
			delete object;
			object = nullptr;
		}
	}

	T* operator->() {
		return object;
	}

	T& operator*() {
		return *object;
	}

	template<class ...Args>
	static void init(Args&& ...args) {
		Global<T>::provide(new T(std::forward<Args>(args)...));
	}

private:
	static inline T* object = nullptr;
};
