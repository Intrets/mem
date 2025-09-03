#pragma once

#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <tepp/assert.h>
#include <tepp/integers.h>
#include <tepp/tepp.h>

#ifdef DEBUG_BUILD
#define RTTI_CHECKS
#endif

#ifdef RTTI_CHECKS
#include <typeinfo>
#endif

#define DEFAULT_COPY(T) \
	T(const T&) = default; \
	T& operator=(const T&) = default;
#define NO_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const T&) = delete;
#define DEFAULT_MOVE(T) \
	T(T&&) = default; \
	T& operator=(T&&) = default;
#define NO_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete;
#define DEFAULT_COPY_MOVE(T) DEFAULT_COPY(T) DEFAULT_MOVE(T)
#define NO_COPY_MOVE(T) NO_COPY(T) NO_MOVE(T)

template<class B>
class ReferenceManager;

using Handle = integer_t;

namespace detail
{
	template<class B>
	struct SafetyChecks
	{
		ReferenceManager<B>* man = nullptr;
		qualifier_t qualifier{};
		Handle handle{};

		void clear() {
			this->man = nullptr;
			this->qualifier = {};
			this->handle = 0;
		}

		void set(B* ptr) {
			if (ptr == nullptr) {
				this->clear();
			}
			else {
				this->man = ptr->referenceManager;
				this->qualifier = qualifier_t(ptr->uniqueIdentifier);
				this->handle = ptr->selfHandle;
			}
		}

		bool safety_check(B const* ptr) const {
			if (ptr == nullptr) {
				return true;
			}

			tassert(this->man != nullptr);
			return this->man->isQualified(this->handle, this->qualifier) && this->man->getPtr(this->handle) == ptr;
		}
	};

	template<class T>
	struct Ptr
	{
		T* ptr = nullptr;

		NO_COPY(Ptr);

		template<class B>
		Ptr(Ptr<B>&& other) {
			this->ptr = other.ptr;
			other.ptr = nullptr;
		}

		template<class B>
		Ptr& operator=(Ptr<B>&& other) {
			this->destroy();

			this->ptr = other.ptr;
			other.ptr = nullptr;

			return *this;
		}

		T* operator->() {
			return this->ptr;
		}

		T const* operator->() const {
			return this->ptr;
		}

		Ptr(std::nullptr_t) {
		}

		template<class... Args>
		Ptr(Args&&... args) {
			this->ptr = new T(std::forward<Args>(args)...);
		}

		void destroy() {
			if (this->ptr == nullptr) {
				return;
			}

			delete this->ptr;

			this->ptr = nullptr;
		}

		~Ptr() {
			this->destroy();
		}
	};
}

template<class B>
class Reference
{
private:
#ifdef DEBUG_BUILD
	detail::SafetyChecks<B> safety{};
#endif

	B* ptr = nullptr;

public:
	bool operator==(Reference const& other) const;
	bool operator==(void* other) const;

	bool operator<(Reference const& other) const;

	bool isNotNull() const;
	bool isNull() const;

	void clearPtr();
	B* getPtr();
	B const* getPtr() const;
	void setPtr(B* ptr_);

	operator bool() const;
};

template<class, class>
class QualifiedReference;

template<class B, class T = B>
class WeakReference : public Reference<B>
{
public:
	T* get() const;
	T* operator->() const;

	Handle getHandle() const;

	template<class R>
	R* getAs() const;

	template<class N>
	WeakReference<B, N> as() const;

	template<class N>
	operator WeakReference<B, N>() const;

	QualifiedReference<B, T> getQualified(ReferenceManager<B>& manager) const;

	void deleteObject(ReferenceManager<B>& manager);
	void clear();

	DEFAULT_COPY_MOVE(WeakReference);

	WeakReference() = default;
	~WeakReference() = default;

	WeakReference(T* p);
	WeakReference(T& o);
	WeakReference(ReferenceManager<B>& manager, Handle h);
};

namespace detailmem::has_unique_identifier_member
{
	template<class T>
	concept helper =
	    std::constructible_from<qualifier_t, std::remove_cvref_t<T>> &&
	    std::constructible_from<std::remove_cvref_t<T>, qualifier_t>;
}

template<class T>
concept has_unique_identifier_member =
    requires(T t) {
	    { t.uniqueIdentifier } -> detailmem::has_unique_identifier_member::helper;
    };

template<class, class>
class UniqueReference;

template<class B, class T = B>
class QualifiedReference : private WeakReference<B, T>
{
public:
	ReferenceManager<B>* manager{};
	qualifier_t qualifier{};
	Handle handle{};

	WeakReference<B, T> getRef() const;
	WeakReference<B, T> getIfValid() const;

	template<class N>
	operator QualifiedReference<B, N>() const;

	bool isValid() const;

	void set(ReferenceManager<B>& manager, WeakReference<B, T> r);
	void set(UniqueReference<B, T>& r);
	void unset();

	QualifiedReference() = default;

	DEFAULT_COPY_MOVE(QualifiedReference);
};

template<class B, class T>
class UniqueReference : public WeakReference<B, T>
{
public:
	ReferenceManager<B>* manager{};

	ReferenceManager<B>* getManager() const;

	QualifiedReference<B, T> getQualified() const;

	template<class S>
	UniqueReference<B, S> convert();

	UniqueReference(ReferenceManager<B>& manager, WeakReference<B, T> ref);
	UniqueReference(ReferenceManager<B>& manager, T* p);
	UniqueReference(ReferenceManager<B>& manager, Handle h);

	virtual ~UniqueReference();

	template<class N>
	UniqueReference(UniqueReference<B, N>&& other);

	template<class N>
	UniqueReference<B, T>& operator=(UniqueReference<B, N>&& other);

	WeakReference<B, T> getWeak() const;

	NO_COPY(UniqueReference);
};

template<class B>
class ReferenceManager
{
private:
	void freeData(Handle h);
	Handle getFreeHandle();

	std::vector<std::pair<Handle, Reference<B>*>> incomplete;
	std::vector<std::pair<Handle, B**>> incompletePointers;

public:
	void addIncomplete(Handle h, Reference<B>* ptr);
	void addIncomplete(Handle h, B*& ptr);
	void completeReferences();

	std::vector<qualifier_t> identifiers{};
	std::vector<detail::Ptr<B>> data{};
	qualifier_t uniqueIdentifierCounter = 3;

	std::vector<Handle> freed{};

	bool validHandle(Handle h) const;

	template<class T = B>
	T* getPtr(Handle h);

private:
	template<class T>
	WeakReference<B, T> storeRef(detail::Ptr<T> ptr);

public:
	template<class T, class... Args>
	WeakReference<B, T> makeRef(Args&&... args);

	template<class T, class... Args>
	UniqueReference<B, T> makeUniqueRef(Args&&... args);

	void deleteReference(Handle h);

	template<class T>
	void deleteReference(WeakReference<B, T>& ref);

	template<class T>
	void deleteReference(UniqueReference<B, T>& ref);

	bool isQualified(Handle handle, qualifier_t qualifier);

	void clear();

	ReferenceManager();
	~ReferenceManager();

	NO_COPY_MOVE(ReferenceManager);
};

template<class B, class T>
inline ReferenceManager<B>* UniqueReference<B, T>::getManager() const {
	return this->manager;
}

template<class B, class T>
inline QualifiedReference<B, T> UniqueReference<B, T>::getQualified() const {
	QualifiedReference<B, T> result{};
	result.set(this->getManager(), this->getWeak());
	return result;
}

template<class B, class T>
inline T* WeakReference<B, T>::get() const {
	return static_cast<T*>(const_cast<B*>(this->getPtr()));
}

template<class B, class T>
inline T* WeakReference<B, T>::operator->() const {
	return this->get();
}

template<class B, class T>
inline Handle WeakReference<B, T>::getHandle() const {
	tassert(this->isNotNull());
	return this->get()->selfHandle;
}

template<class B>
inline Reference<B>::operator bool() const {
	return this->isNotNull();
}

template<class B, class T>
inline QualifiedReference<B, T> WeakReference<B, T>::getQualified(ReferenceManager<B>& manager) const {
	QualifiedReference<B, T> result{};
	result.set(manager, *this);
	return result;
}

template<class B, class T>
inline void WeakReference<B, T>::deleteObject(ReferenceManager<B>& manager) {
	if (this->isNotNull()) {
		manager.deleteReference(this->getHandle());
		this->clearPtr();
	}
}

template<class B, class T>
inline void WeakReference<B, T>::clear() {
	this->clearPtr();
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(T* p) {
	this->setPtr(p);
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(T& o) {
	this->setPtr(&o);
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(ReferenceManager<B>& manager_, Handle h) {
	this->setPtr(manager_.getPtr(h));
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<T*>(this->get()));
#endif
}

template<class B>
template<class T>
inline T* ReferenceManager<B>::getPtr(Handle h) {
	if (this->validHandle(h)) {
		return static_cast<T*>(this->data[h].ptr);
	}
	else {
		return nullptr;
	}
}

template<class B>
template<class T>
inline WeakReference<B, T> ReferenceManager<B>::storeRef(detail::Ptr<T> object) {
	Handle h = this->getFreeHandle();

	auto ptr = object.ptr;

	this->data[h] = std::move(object);

	ptr->selfHandle = h;
	ptr->referenceManager = this;

	if constexpr (has_unique_identifier_member<B>) {
		ptr->uniqueIdentifier = decltype(ptr->uniqueIdentifier)(this->uniqueIdentifierCounter++);
		this->identifiers[h] = qualifier_t(ptr->uniqueIdentifier);
	}

	return WeakReference<B, T>(ptr);
}

template<class B>
template<class T, class... Args>
inline WeakReference<B, T> ReferenceManager<B>::makeRef(Args&&... args) {
	auto ptr = detail::Ptr<T>(std::forward<Args>(args)...);

	return this->storeRef(std::move(ptr));
}

template<class B>
template<class T, class... Args>
inline UniqueReference<B, T> ReferenceManager<B>::makeUniqueRef(Args&&... args) {
	return UniqueReference<B, T>(*this, this->makeRef<T>(std::forward<Args>(args)...).get());
}

template<class B>
template<class T>
inline void ReferenceManager<B>::deleteReference(WeakReference<B, T>& ref) {
	this->deleteReference(ref.getHandle());
	ref.clear();
}

template<class B>
template<class T>
inline void ReferenceManager<B>::deleteReference(UniqueReference<B, T>& ref) {
	this->deleteReference(ref.getHandle());
	ref.clear();
}

template<class B>
inline void ReferenceManager<B>::deleteReference(Handle h) {
	if (h == 0) {
		return;
	}

	this->freeData(h);
}

template<class B>
inline bool ReferenceManager<B>::isQualified(Handle handle, qualifier_t qualifier) {
	tassert(handle > 0);
	tassert(handle < isize(this->identifiers));
	return handle > 0 && handle < isize(this->identifiers) && this->identifiers[handle] == qualifier;
}

template<class B>
inline void ReferenceManager<B>::clear() {
	this->incomplete.resize(0);
	this->incompletePointers.resize(0);

	this->data.clear();
	this->data.emplace_back(nullptr);
	this->identifiers.clear();
	this->identifiers.push_back({});

	this->freed.clear();
}

template<class B>
inline ReferenceManager<B>::ReferenceManager() {
	this->data.emplace_back(nullptr);
	this->identifiers.push_back({});
}

template<class B>
inline ReferenceManager<B>::~ReferenceManager() {
	this->clear();
}

template<class B>
inline void ReferenceManager<B>::freeData(Handle h) {
	this->data[h].destroy();

	this->identifiers[h] = qualifier_t(2);
	this->freed.push_back(h);
}

template<class B>
inline Handle ReferenceManager<B>::getFreeHandle() {
	if (!this->freed.empty()) {
		auto handle = this->freed.back();
		this->freed.pop_back();

		return handle;
	}
	else {
		auto handle = isize(this->data);
		this->data.emplace_back(nullptr);
		this->identifiers.push_back({});

		return handle;
	}
}

template<class B>
inline void ReferenceManager<B>::addIncomplete(Handle h, Reference<B>* ptr) {
	this->incomplete.push_back({ h, ptr });
}

template<class B>
inline void ReferenceManager<B>::addIncomplete(Handle h, B*& ptr) {
	this->incompletePointers.push_back({ h, &ptr });
}

template<class B>
inline void ReferenceManager<B>::completeReferences() {
	for (auto [h, ptr] : this->incomplete) {
		ptr->ptr = this->data[h].get();
	}
	this->incomplete.clear();

	for (auto [h, ptr] : this->incompletePointers) {
		*ptr = this->data[h].get();
	}
	this->incompletePointers.clear();
}

template<class B>
inline bool ReferenceManager<B>::validHandle(Handle h) const {
	if (h >= 0 && h < isize(this->data)) {
		return this->identifiers[h] != 0;
	}
	else {
		return false;
	}
}

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager_, WeakReference<B, T> ref)
    : WeakReference<B, T>(ref) {
	this->manager = &manager_;
}

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager_, T* p)
    : WeakReference<B, T>(p) {
	this->manager = &manager_;
}

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager_, Handle h)
    : WeakReference<B, T>(manager_, h) {
	this->manager = &manager_;
}

template<class B, class T>
inline UniqueReference<B, T>::~UniqueReference() {
	if (this->isNotNull()) {
		this->getManager()->deleteReference(this->getHandle());
		this->clearPtr();
		this->manager = nullptr;
	}
}

template<class B, class T>
inline WeakReference<B, T> UniqueReference<B, T>::getWeak() const {
	return *this;
}

template<class B, class T>
template<class S>
inline UniqueReference<B, S> UniqueReference<B, T>::convert() {
	return std::move(*this);
}

template<class B, class T>
template<class N>
inline UniqueReference<B, T>::UniqueReference(UniqueReference<B, N>&& other) {
	tassert(other.isNotNull());
	tassert(this->manager == nullptr || this->manager == other.manager);

	this->setPtr(other.getPtr());
	this->manager = other.manager;

	other.clearPtr();
	other.manager = nullptr;
}

template<class B, class T>
template<class N>
inline UniqueReference<B, T>& UniqueReference<B, T>::operator=(UniqueReference<B, N>&& other) {
	if constexpr (std::is_same<T, N>::value) {
		if (this == &other) {
			return *this;
		}
	}
	tassert(this->getPtr() != other.getPtr());
	tassert(this->manager == nullptr || this->manager == other.manager);

	if (this->manager) {
		this->deleteObject(*this->manager);
	}

	this->setPtr(other.getPtr());
	this->manager = other.manager;
	other.clearPtr();
	other.manager = nullptr;
	return *this;
}

template<class B, class T>
template<class R>
inline R* WeakReference<B, T>::getAs() const {
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<R*>(this->get()));
#endif
	return static_cast<R*>(const_cast<B*>(this->getPtr()));
}

template<class B, class T>
template<class N>
inline WeakReference<B, N> WeakReference<B, T>::as() const {
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<N*>(this->get()));
#endif
	return WeakReference<B, N>(reinterpret_cast<N*>(this->get()));
}

template<class B, class T>
template<class N>
inline WeakReference<B, T>::operator WeakReference<B, N>() const {
	static_assert(std::is_base_of<N, T>::value, "WeakReference implicit cast: not a super class.");
	return WeakReference<B, N>(this->get());
}

template<class B>
inline bool Reference<B>::operator==(Reference<B> const& other) const {
	return this->ptr == other.ptr;
}

template<class B>
inline bool Reference<B>::operator==(void* other) const {
	return this->ptr == other;
}

template<class B>
inline bool Reference<B>::operator<(Reference<B> const& other) const {
	return this->ptr < other.ptr;
}

template<class B>
inline bool Reference<B>::isNotNull() const {
	return this->ptr != nullptr;
}

template<class B>
inline bool Reference<B>::isNull() const {
	return this->ptr == nullptr;
}

template<class B>
inline void Reference<B>::clearPtr() {
	this->ptr = nullptr;

#ifdef DEBUG_BUILD
	this->safety.clear();
#endif
}

template<class B>
inline B* Reference<B>::getPtr() {
#ifdef DEBUG_BUILD
	tassert(this->safety.safety_check(this->ptr));
#endif

	return this->ptr;
}

template<class B>
inline B const* Reference<B>::getPtr() const {
#ifdef DEBUG_BUILD
	tassert(this->safety.safety_check(this->ptr));
#endif

	return this->ptr;
}

template<class B, class T>
inline WeakReference<B, T> QualifiedReference<B, T>::getRef() const {
	return *this;
}

template<class B, class T>
inline WeakReference<B, T> QualifiedReference<B, T>::getIfValid() const {
	if (this->isValid()) {
		return this->getRef();
	}
	else {
		return WeakReference<B, T>();
	}
}

template<class B, class T>
inline bool QualifiedReference<B, T>::isValid() const {
	if (this->manager == nullptr) {
		return false;
	}

	return this->manager->isQualified(this->handle, this->qualifier);
}

template<class B, class T>
inline void QualifiedReference<B, T>::set(ReferenceManager<B>& manager_, WeakReference<B, T> r) {
	tassert(this->manager == nullptr || this->manager == &manager_);

	this->manager = &manager_;
	this->handle = r.getHandle();
	this->setPtr(r.get());
	this->qualifier = qualifier_t(r->uniqueIdentifier);
}

template<class B, class T>
inline void QualifiedReference<B, T>::set(UniqueReference<B, T>& r) {
	this->set(*r.getManager(), r.getWeak());
}

template<class B, class T>
inline void QualifiedReference<B, T>::unset() {
	this->qualifier = qualifier_t(1);
}

template<class B, class T>
template<class N>
inline QualifiedReference<B, T>::operator QualifiedReference<B, N>() const {
	QualifiedReference<B, N> result{};
	result.set(*this->manager, this->getRef());
	return result;
}

template<class B>
inline void Reference<B>::setPtr(B* ptr_) {
#ifdef DEBUG_BUILD
	this->safety.set(ptr_);
#endif

	this->ptr = ptr_;
}
