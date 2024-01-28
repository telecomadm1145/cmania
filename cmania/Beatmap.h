#pragma once
#include "Defines.h"
#include "HitObject.h"

class Beatmap {
public:
	virtual ~Beatmap() {
	}
	virtual std::string RulesetId() const noexcept = 0;
	virtual std::string Title() const noexcept = 0;
	virtual std::string Version() const noexcept = 0;
	virtual Hash BeatmapHashcode() const noexcept = 0;
	virtual size_t MaxCombo() const noexcept = 0;
	virtual double FirstObject() const noexcept = 0;
	virtual double Length() const noexcept = 0;
	virtual size_t size() const noexcept = 0;
	virtual void* GetBeatmap() = 0;
	virtual HitObject& at(size_t i) const = 0;
	virtual path BgPath() const noexcept = 0;
	virtual path BgmPath() const noexcept = 0;
	virtual double GetDifficultyValue(std::string key) const noexcept = 0;
	virtual std::unordered_set<std::string> GetDifficultyValues() const noexcept = 0;
	HitObject& operator[](size_t i) const {
		return at(i);
	}
	template <class ParentType, class HitObject>
	class iterator {
	public:
		ParentType* bmp;
		size_t i;
		iterator& operator++() {
			i++;
			return *this;
		}
		iterator& operator++(int) {
			i++;
			return *this;
		}
		iterator& operator--() {
			i--;
			return *this;
		}
		iterator& operator--(int) {
			i--;
			return *this;
		}
		template <std::integral T>
		iterator& operator+(T j) {
			i += j;
			return *this;
		}
		template <std::integral T>
		iterator& operator-(T j) {
			i -= j;
			return *this;
		}
		HitObject& operator*() {
			return bmp->at(i);
		}
		HitObject* operator->() {
			return &bmp->at(i);
		}
		bool operator==(const iterator& rhs) const {
			return i == rhs.i;
		}
		bool operator!=(const iterator& rhs) const {
			return i != rhs.i;
		}
	};
	auto begin() {
		return iterator<Beatmap, HitObject>{ this, 0 };
	}
	auto end() {
		return iterator<Beatmap, HitObject>{ this, size() };
	}
	auto cbegin() const {
		return iterator<const Beatmap, HitObject>{ this, 0 };
	}
	auto cend() const {
		return iterator<const Beatmap, HitObject>{ this, size() };
	}
	template <class RulesetHitObject>
	auto super(){
		class SuperClass {
		public:
			Beatmap* bmp;
			RulesetHitObject& at(size_t i) const {
				return (RulesetHitObject&)bmp->at(i);
			}
			size_t size() const {
				return bmp->size();
			}
			RulesetHitObject& operator[](size_t i) const {
				return (RulesetHitObject&)bmp->at(i);
			}
			auto begin() {
				return iterator<SuperClass, RulesetHitObject>{ this, 0 };
			}
			auto end() {
				return iterator<SuperClass, RulesetHitObject>{ this, size() };
			}
			auto cbegin() const {
				return iterator<const SuperClass, RulesetHitObject>{ this, 0 };
			}
			auto cend() const {
				return iterator<const SuperClass, RulesetHitObject>{ this, size() };
			}
		};
		return SuperClass{ this };
	}
};