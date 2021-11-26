// mem - A C++ library for managing objects
// Copyright (C) 2021  Intrets
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <type_traits>
#include <optional>
#include <iostream>
#include <algorithm>
#include <cassert>

#ifdef RTTI_CHECKS
#include <typeinfo>
#endif

#define DEFAULT_COPY(T) T(const T&) = default; T& operator=(const T&) = default;
#define NO_COPY(T) T(const T&) = delete; T& operator=(const T&) = delete;
#define DEFAULT_MOVE(T) T(T&&) = default; T& operator=(T&&) = default;
#define NO_MOVE(T) T(T&&) = delete; T& operator=(T&&) = delete;
#define DEFAULT_COPY_MOVE(T) DEFAULT_COPY(T) DEFAULT_MOVE(T)
#define NO_COPY_MOVE(T) NO_COPY(T) NO_MOVE(T)

using Handle = int32_t;

template <class B>
class ReferenceManager;

class Reference
{
public:
	void* ptr = nullptr;
	void* manager = nullptr;

	bool operator==(Reference const& other) const;
	bool operator==(void* other) const;

	bool isNotNull() const;
	bool isNull() const;

	void clearPtr();

	operator bool() const;

	virtual ~Reference() = default;
};

template <class B, class T>
class WeakReference : public Reference
{
public:
	ReferenceManager<B>* getManager() const;

	T* get() const;

	Handle getHandle() const;

	template<class R>
	R* getAs() const;

	template<class N>
	WeakReference<B, N> as() const;

	template<class N>
	operator WeakReference<B, N>() const;

	void deleteObject();
	void clear();

	WeakReference() = default;

	WeakReference(ReferenceManager<B>& manager, B* p);
	WeakReference(ReferenceManager<B>& manager, Handle h);

	virtual ~WeakReference() = default;
};

template<class B, class T>
class UniqueReference : public WeakReference<B, T>
{
public:
	UniqueReference() = default;

	UniqueReference(WeakReference<B, T> ref);
	UniqueReference(ReferenceManager<B>& manager, B* p);
	UniqueReference(ReferenceManager<B>& manager, Handle h);

	UniqueReference<B, T>& operator= (WeakReference<B, T> const& other);

	virtual ~UniqueReference();

	template<class N>
	UniqueReference(UniqueReference<B, N>&& other);

	template<class N>
	UniqueReference<B, T>& operator= (UniqueReference<B, N>&& other);

	WeakReference<B, T> getWeak() const;

	NO_COPY(UniqueReference);
};

template <class B, class T>
class ManagedReference : private WeakReference<B, T>
{
private:
	friend class ReferenceManager<B>;

public:
	WeakReference<B, T> getRef() const;

	void set(WeakReference<B, T> r);

	bool isValid() const;
	void unset();

	ManagedReference() = default;

	ManagedReference(WeakReference<B, T> r);
	ManagedReference(ReferenceManager<B>& manager, Handle h);
	ManagedReference(ReferenceManager<B>& manager, B* p);

	ManagedReference(ManagedReference&& other) noexcept;
	ManagedReference<B, T>& operator= (ManagedReference&& other) noexcept;

	ManagedReference(const ManagedReference&);
	ManagedReference& operator=(const ManagedReference&);

	virtual ~ManagedReference();
};

template <class B>
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

	int32_t size;
	typedef std::unordered_multimap<Handle, Reference*> ManagedReferencesType;

	ManagedReferencesType managedReferences;
	std::unordered_map<Handle, std::unique_ptr<B>> data;
	std::set<Handle> freeHandles;
	bool freeHandlesSorted = true;

	std::vector<int32_t> usedHandle;

	template<class T>
	T* getPtr(Handle h);

	template <class T, class... Args>
	WeakReference<B, T> makeRef(Args&&... args);

	template <class T, class... Args>
	UniqueReference<B, T> makeUniqueRef(Args&&... args);

	bool storeReference(Handle h, B* ref);

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

	void clear();

	ReferenceManager(int32_t size_);
	ReferenceManager() : ReferenceManager(10'000) {
	};
	~ReferenceManager();

	NO_COPY_MOVE(ReferenceManager);
};

template<class B, class T>
inline ReferenceManager<B>* WeakReference<B, T>::getManager() const {
	return static_cast<ReferenceManager<B>*>(this->manager);
}

template<class B, class T>
inline T* WeakReference<B, T>::get() const {
	return static_cast<T*>(this->ptr);
}

template<class B, class T>
inline Handle WeakReference<B, T>::getHandle() const {
	assert(this->isNotNull());
	return this->get()->selfHandle;
}

inline Reference::operator bool() const {
	return this->isNotNull();
}

template<class B, class T>
inline void WeakReference<B, T>::deleteObject() {
	if (this->isNotNull()) {
		this->getManager()->deleteReference(this->getHandle());
		this->ptr = nullptr;
		this->manager = nullptr;
	}
}

template<class B, class T>
inline void WeakReference<B, T>::clear() {
	this->ptr = nullptr;
	this->manager = nullptr;
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(ReferenceManager<B>& manager_, B* p) {
	this->ptr = p;
	this->manager = &manager_;
}

template<class B, class T>
inline WeakReference<B, T>::WeakReference(ReferenceManager<B>& manager_, Handle h) {
	this->ptr = manager_.data[h].get();
	this->manager = &manager_;
#ifdef RTTI_CHECKS
	assert(dynamic_cast<T*>(this->get()));
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
inline ManagedReference<B, T>::ManagedReference(WeakReference<B, T> r) {
	this->set(r);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ReferenceManager<B>& manager, Handle h) {
	auto ref = WeakReference<B, T>(manager, h);
	this->set(ref);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(ReferenceManager<B>& manager, B* p) {
	auto ref = WeakReference<B, T>(manager, p);
	this->set(ref);
}

template<class B, class T>
inline WeakReference<B, T> ManagedReference<B, T>::getRef() const {
	return *this;
}

template<class B, class T>
inline void ManagedReference<B, T>::set(WeakReference<B, T> r) {
	assert(r.manager != nullptr);
	assert(this->manager == nullptr || this->manager == r.manager);

	if (this->isValid()) {
		this->getManager()->unsubscribe(*this);
	}
	else {
		this->manager = r.manager;
	}

	this->ptr = r.get();
	this->getManager()->subscribe(*this);
}

template<class B, class T>
inline ManagedReference<B, T>::ManagedReference(const ManagedReference& other) {
	if (auto ref = other.getRef()) {
		this->set(ref);
	}
}

template<class B, class T>
inline ManagedReference<B, T>& ManagedReference<B, T>::operator=(const ManagedReference& other) {
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
	return static_cast<T*>(data[h].get());
}

template<class B>
template<class T, class ...Args>
inline WeakReference<B, T> ReferenceManager<B>::makeRef(Args&& ...args) {
	Handle h = this->getFreeHandle();
	this->data[h] = std::make_unique<T>(std::forward<Args>(args)...);
	this->data[h]->selfHandle = h;
	this->usedHandle[h] = true;
	auto ptr = this->data[h].get();
	return WeakReference<B, T>(*this, ptr);
}

template<class B>
template<class T, class ...Args>
inline UniqueReference<B, T> ReferenceManager<B>::makeUniqueRef(Args&& ...args) {
	Handle h = this->getFreeHandle();
	this->data[h] = std::make_unique<T>(std::forward<Args>(args)...);
	this->data[h]->selfHandle = h;
	this->usedHandle[h] = true;
	auto ptr = this->data[h].get();
	return UniqueReference<B, T>(*this, ptr);
}

template<class B>
template<class T>
inline void ReferenceManager<B>::subscribe(ManagedReference<B, T>& toManage) {
#ifdef _DEBUG
	auto range = managedReferences.equal_range(toManage.getHandle());

	auto it = range.first;
	auto end = range.second;

	for (; it != end; it++) {
		assert(it->second != static_cast<Reference*>(&toManage));
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

		assert(count == 1);
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
inline bool ReferenceManager<B>::storeReference(Handle h, B* ref) {
	if (freeHandles.count(h) == 0) {
		return false;
	}
	freeHandles.erase(h);
	usedHandle[h] = true;
	data[h] = std::unique_ptr<B>(ref);
	return true;
}

template<class B>
inline void ReferenceManager<B>::deleteReference(Handle h) {
	if (h == 0) {
		return;
	}
	auto range = this->managedReferences.equal_range(h);
	for_each(range.first, range.second, [](ManagedReferencesType::value_type& ref) -> void
		{
			ref.second->clearPtr();
		});

	this->managedReferences.erase(range.first, range.second);

	this->freeData(h);
}

template<class B>
inline void ReferenceManager<B>::clear() {
	this->incomplete.resize(0);
	this->incompletePointers.resize(0);

	for (auto [_, managed] : this->managedReferences) {
		managed->clearPtr();
	}
	this->managedReferences.clear();

	while (!this->data.empty()) {
		this->data.erase(this->data.begin());
	}

	this->freeHandles.clear();
	for (int32_t i = 1; i < this->size; i++) {
		this->freeHandles.insert(i);
	}

	std::fill(this->usedHandle.begin(), this->usedHandle.end(), false);
}

template<class B>
inline ReferenceManager<B>::ReferenceManager(int32_t size_) :
	size(size_),
	usedHandle(size) {
	for (int32_t i = 1; i < this->size; i++) {
		this->freeHandles.insert(i);
	}
}

template<class B>
inline ReferenceManager<B>::~ReferenceManager() {
	this->clear();
}

template<class B>
inline void ReferenceManager<B>::freeData(Handle h) {
	data.erase(h);
	usedHandle[h] = false;
	freeHandles.insert(h);
}

template<class B>
inline Handle ReferenceManager<B>::getFreeHandle() {
	auto it = freeHandles.begin();
	Handle h = *it;
	freeHandles.erase(it);
	return h;
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

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(WeakReference<B, T> ref) : WeakReference<B, T>(ref) {
}

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager, B* p) : WeakReference<B, T>(manager, p) {
}

template<class B, class T>
inline UniqueReference<B, T>::UniqueReference(ReferenceManager<B>& manager_, Handle h) : WeakReference<B, T>(manager_, h) {
}

template<class B, class T>
inline UniqueReference<B, T>& UniqueReference<B, T>::operator=(WeakReference<B, T> const& other) {
	assert(this->isNull());
	assert(this->manager == nullptr || this->manager == other.manager);

	this->ptr = other.ptr;
	this->manager = other.manager;

	return *this;
}

template<class B, class T>
inline UniqueReference<B, T>::~UniqueReference() {
	WeakReference<B, T>::deleteObject();
}

template<class B, class T>
inline WeakReference<B, T> UniqueReference<B, T>::getWeak() const {
	return *this;
}

template<class B, class T>
template<class N>
inline UniqueReference<B, T>::UniqueReference(UniqueReference<B, N>&& other) {
	assert(other.isNotNull());
	assert(this->manager == nullptr || this->manager == other.manager);

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
	assert(this->ptr != other.ptr);
	assert(this->manager == nullptr || this->manager == other.manager);
	assert(other.isNotNull());
	assert(this->isNull());

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
			this->set(ref);
		}
		other.unset();
	}
	return *this;
}

template<class B, class T>
template<class R>
inline R* WeakReference<B, T>::getAs() const {
#ifdef RTTI_CHECKS
	assert(dynamic_cast<R*>(this->get()));
#endif
	return static_cast<R*>(this->ptr);
}

template<class B, class T>
template<class N>
inline WeakReference<B, N> WeakReference<B, T>::as() const {
#ifdef RTTI_CHECKS
	assert(dynamic_cast<N*>(this->get()));
#endif
	return WeakReference<B, N>(*this->getManager(), this->get());
}

template<class B, class T>
template<class N>
inline WeakReference<B, T>::operator WeakReference<B, N>() const {
	static_assert(std::is_base_of<N, T>::value, "WeakReference implicit cast: not a super class.");
	return WeakReference<B, N>(*this->getManager(), this->get());
}

inline bool Reference::operator==(Reference const& other) const {
	return this->ptr == other.ptr;
}

inline bool Reference::operator==(void* other) const {
	return this->ptr == other;
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
