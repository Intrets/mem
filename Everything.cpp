// mem - A C++ library for managing objects
// Copyright (C) 2021 intrets

#include "Everything.h"

namespace mem
{
	UniqueObject::~UniqueObject() {
		if (this->proxy != nullptr) {
			this->proxy->remove(this->index);
		}
	}

	void UniqueObject::release() {
		this->index.set(0);
	}

	UniqueObject::UniqueObject(WeakObject const& other) {
		this->index = other.index;
		this->proxy = other.proxy;
	}

	UniqueObject::UniqueObject(UniqueObject&& other) noexcept {
		this->index = other.index;
		this->proxy = other.proxy;
		other.index.set(0);
		other.proxy = nullptr;
	}

	UniqueObject& UniqueObject::operator=(UniqueObject&& other) noexcept {
		if (this->proxy != nullptr) {
			this->proxy->remove(this->index);
		}

		this->index = other.index;
		this->proxy = other.proxy;
		other.index.set(0);
		other.proxy = nullptr;

		return *this;
	}

	void WeakObject::deleteObject() {
		if (this->isNotNull()) {
			this->proxy->remove(this->index);
		}
	}

	bool WeakObject::isNull() const {
		return (this->index == 0) || (this->proxy == nullptr);
	}

	bool WeakObject::isNotNull() const {
		return (this->index != 0) && (this->proxy != nullptr);
	}

	bool WeakObject::has(Index<Component> i) const {
		tassert(this->isNotNull());
		return this->proxy->has(this->index, i);
	}

	Index<RawData> WeakObject::getComponentIndex(Index<Component> type) const {
		return this->proxy->getComponentIndex(this->index, type);
	}

	bool WeakObject::operator==(WeakObject const& other) {
		return this->index == other.index && this->proxy == other.proxy;
	}

	WeakObject Everything::make() {
		if (!this->freeIndirections.empty()) {
			auto i = this->freeIndirections.back();
			this->freeIndirections.pop_back();

			this->validIndices[i] = true;

			tassert(this->signatures[i].none());

			return { i, this };
		}
		else {
			this->signatures.push_back(0);
			for (size_t type = 0; type < this->getTypeCount(); type++) {
				this->dataIndices[type].push_back(Index<RawData>{ 0 });
			}

			this->qualifiers.push_back(this->getNextQualifier());
			this->validIndices.push_back(true);

			Index<Everything> i{ this->signatures.size() - 1 };

			tassert(this->signatures[i].none());

			return { i, this };
		}
	}

	UniqueObject Everything::makeUnique() {
		return this->make();
	}

	UniqueObject Everything::cloneAll(WeakObject const& obj) {
		std::vector<Index<Component>> all;

		for (size_t i = 0; i < this->getTypeCount(); i++) {
			all.push_back(Index<Component>{ i });
		}

		return this->clone(all, obj);
	}

	UniqueObject Everything::clone(std::vector<Index<Component>> components, WeakObject const& obj) {
		tassert(0);
		auto p = this->makeUnique();
		for (auto& type : components) {
			if (obj.has(type)) {
				auto componentIndex = obj.getComponentIndex(type);
				auto newComponentIndex = this->data[type].cloneUntyped(componentIndex, p.index);

				this->dataIndices[type][p.index] = newComponentIndex;
				this->signatures[p.index].set(type);
			}
		}
		return p;
	}

	std::optional<WeakObject> Everything::maybeGetFromIndex(Index<Everything> index) {
		if (this->isValidIndex(index)) {
			return this->getFromIndex(index);
		}
		else {
			return std::nullopt;
		}
	}

	WeakObject Everything::getFromIndex(Index<Everything> index) {
		tassert(index > 0);
		tassert(this->isValidIndex(index));
		return { index, this };
	}

	bool Everything::isValidIndex(Index<Everything> index) {
		return (index > 0) && this->validIndices[index];
	}

	Qualifier Everything::getNextQualifier() {
		return this->qualifier++;
	}

	bool Everything::isQualified(Index<Everything> i, Qualifier q) const {
		tassert(i != 0);
		return this->qualifiers[i] == q;
	}

	Qualifier Everything::getQualifier(Index<Everything> i) const {
		tassert(i != 0);
		return this->qualifiers[i];
	}

	void QualifiedObject::set(WeakObject obj) {
		tassert(obj.isNotNull());

		this->object = obj;
		this->qualifier = this->object.proxy->getQualifier(this->object.index);
	}

	bool QualifiedObject::isQualified() const {
		return this->object.isNotNull() && this->object.proxy->isQualified(this->object.index, this->qualifier);
	}

	WeakObject* QualifiedObject::operator->() {
		tassert(this->isQualified());

		return &this->object;
	}

	QualifiedObject& QualifiedObject::operator=(WeakObject const& other) noexcept {
		this->set(other);

		return *this;
	}

	QualifiedObject::QualifiedObject(WeakObject const& other) noexcept {
		this->set(other);
	}
}
