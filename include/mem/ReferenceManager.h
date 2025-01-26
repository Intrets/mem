// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

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

class Reference
{
public:
	void* ptr = nullptr;

	bool operator==(Reference const& other) const;
	bool operator==(void* other) const;

	bool operator<(Reference const& other) const;

	bool isNotNull() const;
	bool isNull() const;

	void clearPtr();

	operator bool() const;

	virtual ~Reference() = default;
};

using Handle = integer_t;

template<class, class>
class QualifiedReference;

template<class B, class T = B>
class WeakReference : public Reference
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

	WeakReference() = default;

	WeakReference(B* p);
	WeakReference(B& o);
	WeakReference(ReferenceManager<B>& manager, Handle h);

	virtual ~WeakReference() = default;
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

	~QualifiedReference() = default;
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
	UniqueReference(ReferenceManager<B>& manager, B* p);
	UniqueReference(ReferenceManager<B>& manager, Handle h);

	virtual ~UniqueReference();

	template<class N>
	UniqueReference(UniqueReference<B, N>&& other);

	template<class N>
	UniqueReference<B, T>& operator=(UniqueReference<B, N>&& other);

	WeakReference<B, T> getWeak() const;

	NO_COPY(UniqueReference);
};

template<class B, class T>
class ManagedReference : private WeakReference<B, T>
{
private:
	friend class ReferenceManager<B>;

public:
	ReferenceManager<B>* manager{};

	ReferenceManager<B>* getManager() const;

	WeakReference<B, T> getRef() const;
	WeakReference<B, T> getIfValid() const;

	void set(ReferenceManager<B>& manager, WeakReference<B, T> r);

	bool isValid() const;
	void unset();

	ManagedReference() = default;

	ManagedReference(ReferenceManager<B>& manager, WeakReference<B, T> r);
	ManagedReference(ReferenceManager<B>& manager, Handle h);
	ManagedReference(ReferenceManager<B>& manager, B* p);

	ManagedReference(ManagedReference&& other) noexcept;
	ManagedReference<B, T>& operator=(ManagedReference&& other) noexcept;

	ManagedReference(ManagedReference const&);
	ManagedReference& operator=(ManagedReference const&);

	virtual ~ManagedReference();
};

template<class B>
class ReferenceManager
{
private:
	void freeData(Handle h);
	Handle getFreeHandle();

	std::vector<std::pair<Handle, Reference*>> incomplete;
	std::vector<std::pair<Handle, B**>> incompletePointers;

public:
	void addIncomplete(Handle h, Reference* ptr);
	void addIncomplete(Handle h, B*& ptr);
	void completeReferences();

	typedef std::unordered_multimap<Handle, Reference*> ManagedReferencesType;

	ManagedReferencesType managedReferences;

	std::vector<qualifier_t> identifiers{};
	std::vector<std::unique_ptr<B>> data{};
	qualifier_t uniqueIdentifierCounter = 3;

	std::vector<Handle> freed{};

	bool validHandle(Handle h) const;

	template<class T = B>
	T* getPtr(Handle h);

	template<class T>
	WeakReference<B, T> storeRef(std::unique_ptr<T> object);

	template<class T, class... Args>
	WeakReference<B, T> makeRef(Args&&... args);

	template<class T>
	UniqueReference<B, T> storeUniqueRef(std::unique_ptr<T> object);

	template<class T, class... Args>
	UniqueReference<B, T> makeUniqueRef(Args&&... args);

	template<class T>
	void subscribe(ManagedReference<B, T>& toManage);
	template<class T>
	void unsubscribe(ManagedReference<B, T>& managedReference);

	void deleteReference(Handle h);

	template<class T>
	void deleteReference(WeakReference<B, T>& ref);

	template<class T>
	void deleteReference(UniqueReference<B, T>& ref);

	template<class T>
	void deleteReference(ManagedReference<B, T>& ref);

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
	return static_cast<T*>(this->ptr);
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

inline Reference::operator bool() const {
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
		this->ptr = nullptr;
	}
}

template<class B, class T>
inline void WeakReference<B, T>::clear() {
	this->ptr = nullptr;
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(B* p) {
	this->ptr = p;
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(B& o) {
	this->ptr = &o;
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(ReferenceManager<B>& manager_, Handle h) {
	this->ptr = manager_.data[h].get();
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<T*>(this->get()));
#endif
}

template<class B, class T>
inline bool ManagedReference<B, T>::isValid() const {
	return this->isNotNull();
}

template<class B, class T>
inline void ManagedReference<B, T>::unset() {
	if (this->isValid()) {
		this->getManager()->unsubscribe(*this);
	}
	this->clear();
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ReferenceManager<B>& manager, WeakReference<B, T> r) {
	this->set(r);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ReferenceManager<B>& manager, Handle h) {
	auto ref = WeakReference<B, T>(manager, h);
	this->set(manager, ref);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ReferenceManager<B>& manager, B* p) {
	auto ref = WeakReference<B, T>(p);
	this->set(manager, ref);
}

template<class B, class T>
inline ReferenceManager<B>* ManagedReference<B, T>::getManager() const {
	return this->manager;
}

template<class B, class T>
inline WeakReference<B, T> ManagedReference<B, T>::getRef() const {
	return *this;
}

template<class B, class T>
inline WeakReference<B, T> ManagedReference<B, T>::getIfValid() const {
	if (this->isValid()) {
		return this->getRef();
	}
	else {
		return WeakReference<B, T>();
	}
}

template<class B, class T>
inline void ManagedReference<B, T>::set(ReferenceManager<B>& manager_, WeakReference<B, T> r) {
	tassert(this->manager == nullptr || this->manager == &manager_);

	if (this->isValid()) {
		this->getManager()->unsubscribe(*this);
	}
	else {
		this->manager = &manager_;
	}

	this->ptr = r.get();
	this->getManager()->subscribe(*this);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ManagedReference const& other) {
	if (auto ref = other.getRef()) {
		this->set(ref);
	}
}

template<class B, class T>
inline ManagedReference<B, T>& ManagedReference<B, T>::operator=(ManagedReference const& other) {
	if (this != &other) {
		if (auto ref = other.getRef()) {
			this->set(ref);
		}
	}
	return *this;
}

template<class B, class T>
inline ManagedReference<B, T>::~ManagedReference() {
	if (this->isValid()) {
		this->getManager()->unsubscribe(*this);
	}
}

template<class B>
template<class T>
inline T* ReferenceManager<B>::getPtr(Handle h) {
	if (this->validHandle(h)) {
		return static_cast<T*>(this->data[h].get());
	}
	else {
		return nullptr;
	}
}

template<class B>
template<class T>
inline WeakReference<B, T> ReferenceManager<B>::storeRef(std::unique_ptr<T> object) {
	Handle h = this->getFreeHandle();

	auto ptr = object.get();

	this->data[h] = std::move(object);

	ptr->selfHandle = h;

	if constexpr (has_unique_identifier_member<B>) {
		ptr->uniqueIdentifier = decltype(ptr->uniqueIdentifier)(this->uniqueIdentifierCounter++);
		this->identifiers[h] = qualifier_t(ptr->uniqueIdentifier);
	}

	return WeakReference<B, T>(ptr);
}

template<class B>
template<class T, class... Args>
inline WeakReference<B, T> ReferenceManager<B>::makeRef(Args&&... args) {
	auto object = std::make_unique<T>(std::forward<Args>(args)...);

	return this->storeRef(std::move(object));
}

template<class B>
template<class T>
inline UniqueReference<B, T> ReferenceManager<B>::storeUniqueRef(std::unique_ptr<T> obj) {
	return UniqueReference<B, T>(*this, this->storeRef<T>(std::move(obj)));
}

template<class B>
template<class T, class... Args>
inline UniqueReference<B, T> ReferenceManager<B>::makeUniqueRef(Args&&... args) {
	return UniqueReference<B, T>(*this, this->makeRef<T>(std::forward<Args>(args)...).get());
}

template<class B>
template<class T>
inline void ReferenceManager<B>::subscribe(ManagedReference<B, T>& toManage) {
#ifdef _DEBUG
	auto range = managedReferences.equal_range(toManage.getHandle());

	auto it = range.first;
	auto end = range.second;

	for (; it != end; it++) {
		tassert(it->second != static_cast<Reference*>(&toManage));
	}
#endif

	this->managedReferences.insert(std::make_pair(toManage.getHandle(), static_cast<Reference*>(&toManage)));
}

template<class B>
template<class T>
inline void ReferenceManager<B>::unsubscribe(ManagedReference<B, T>& managedReference) {
	auto range = this->managedReferences.equal_range(managedReference.getHandle());

#ifdef _DEBUG
	{
		auto it = range.first;
		auto end = range.second;
		int32_t count = 0;

		for (; it != end; it++) {
			if (it->second == static_cast<Reference*>(&managedReference)) {
				count++;
			}
		}

		tassert(count == 1);
	}
#endif

	auto it = range.first;
	auto end = range.second;

	for (; it != end; it++) {
		if (it->second == static_cast<Reference*>(&managedReference)) {
			it = managedReferences.erase(it);
			break;
		}
	}
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
template<class T>
inline void ReferenceManager<B>::deleteReference(ManagedReference<B, T>& ref) {
	this->deleteReference(ref.getHandle());
	ref.clear();
}

template<class B>
inline void ReferenceManager<B>::deleteReference(Handle h) {
	if (h == 0) {
		return;
	}
	auto range = this->managedReferences.equal_range(h);
	for_each(range.first, range.second, [](ManagedReferencesType::value_type& ref) -> void {
		ref.second->clearPtr();
	});

	this->managedReferences.erase(range.first, range.second);

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

	for (auto [_, managed] : this->managedReferences) {
		managed->clearPtr();
	}
	this->managedReferences.clear();

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
	this->data[h].reset();
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
inline void ReferenceManager<B>::addIncomplete(Handle h, Reference* ptr) {
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
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager_, B* p)
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
		this->ptr = nullptr;
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

	this->ptr = other.ptr;
	this->manager = other.manager;

	other.ptr = nullptr;
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
	tassert(this->ptr != other.ptr);
	tassert(this->manager == nullptr || this->manager == other.manager);

	if (this->manager) {
		this->deleteObject(*this->manager);
	}

	this->ptr = other.ptr;
	this->manager = other.manager;
	other.ptr = nullptr;
	other.manager = nullptr;
	return *this;
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ManagedReference&& other) noexcept {
	if (auto ref = other.getRef()) {
		this->set(ref);
	}
	other.unset();
}

template<class B, class T>
inline ManagedReference<B, T>& ManagedReference<B, T>::operator=(ManagedReference&& other) noexcept {
	if (this != &other) {
		if (auto ref = other.getRef()) {
			this->set(*other.manager, ref);
		}
		other.unset();
	}
	return *this;
}

template<class B, class T>
template<class R>
inline R* WeakReference<B, T>::getAs() const {
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<R*>(this->get()));
#endif
	return static_cast<R*>(this->ptr);
}

template<class B, class T>
template<class N>
inline WeakReference<B, N> WeakReference<B, T>::as() const {
#ifdef RTTI_CHECKS
	tassert(dynamic_cast<N*>(this->get()));
#endif
	return WeakReference<B, N>(this->get());
}

template<class B, class T>
template<class N>
inline WeakReference<B, T>::operator WeakReference<B, N>() const {
	static_assert(std::is_base_of<N, T>::value, "WeakReference implicit cast: not a super class.");
	return WeakReference<B, N>(this->get());
}

inline bool Reference::operator==(Reference const& other) const {
	return this->ptr == other.ptr;
}

inline bool Reference::operator==(void* other) const {
	return this->ptr == other;
}

inline bool Reference::operator<(Reference const& other) const {
	return this->ptr < other.ptr;
}

inline bool Reference::isNotNull() const {
	return this->ptr != nullptr;
}

inline bool Reference::isNull() const {
	return this->ptr == nullptr;
}

inline void Reference::clearPtr() {
	this->ptr = nullptr;
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
	this->ptr = r.get();
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
