#pragma once

#include <utility>

template<class T>
class Global
{
public:
	static T* get() {
		return object;
	};
	static T& ref() {
		return *object;
	};

	static void provide(T* obj) {
		if (object != nullptr) {
			delete object;
		}
		object = obj;
	}

	static void destroy() {
		if (object != nullptr) {
			delete object;
			object = nullptr;
		}
	}

	template<class ...Args>
	static void init(Args&& ...args) {
		T* o = new T(std::forward<Args>(args)...);
		Global<T>::provide(o);
	}

private:
	static T* object;
};

template<class T> T* Global<T>::object = nullptr;
