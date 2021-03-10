#pragma once

template<class T>
class Locator
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

private:
	static T* object;
};

template<class T> T* Locator<T>::object;
