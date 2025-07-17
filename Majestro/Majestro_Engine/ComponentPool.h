#pragma once

#include "Entity.h"

class BaseComponentPool {
public:
    virtual ~BaseComponentPool() = default;
    virtual void RemoveComponent(EntityID id) = 0;
};


template<typename T>
class ComponentPool : public BaseComponentPool {
public:
    ComponentPool() {
        mComponents.reserve(100);  // 초기 용량 확보
        mEntityToIndex.reserve(100);
    }

    // 컴포넌트 추가
    void AddComponent(EntityID entity, T&& component) {
        if (mEntityToIndex.find(entity) != mEntityToIndex.end()) {
            // 이미 존재하면 업데이트
            size_t index = mEntityToIndex[entity];
            mComponents[index] = std::move(component);
        }
        else {
            // 새로 추가
            size_t index = mComponents.size();
            mComponents.emplace_back(std::move(component));
            mEntityToIndex[entity] = index;
            mIndexToEntity.emplace_back(entity);
        }
    }

    // 컴포넌트 제거
    void RemoveComponent(EntityID entity) {
        auto it = mEntityToIndex.find(entity);
        if (it == mEntityToIndex.end()) return;

        size_t indexToRemove = it->second;
        size_t lastIndex = mComponents.size() - 1;

        if (indexToRemove != lastIndex) {
            // 마지막 요소를 제거할 위치로 이동
            mComponents[indexToRemove] = std::move(mComponents[lastIndex]);
            EntityID lastEntity = mIndexToEntity[lastIndex];
            mEntityToIndex[lastEntity] = indexToRemove;
            mIndexToEntity[indexToRemove] = lastEntity;
        }

        // 마지막 요소 제거
        mComponents.pop_back();
        mIndexToEntity.pop_back();
        mEntityToIndex.erase(entity);
    }

    // 컴포넌트 가져오기
    T* GetComponent(EntityID entity) {
        auto it = mEntityToIndex.find(entity);
        if (it != mEntityToIndex.end()) {
            return &mComponents[it->second];
        }
        return nullptr;
    }

    const T* GetComponent(EntityID entity) const {
        auto it = mEntityToIndex.find(entity);
        if (it != mEntityToIndex.end()) {
            return &mComponents[it->second];
        }
        return nullptr;
    }

    // 컴포넌트 존재 여부 확인
    bool HasComponent(EntityID entity) const {
        return mEntityToIndex.find(entity) != mEntityToIndex.end();
    }

    // 모든 컴포넌트에 대한 반복자 지원
    auto begin() { return mComponents.begin(); }
    auto end() { return mComponents.end(); }
    auto begin() const { return mComponents.begin(); }
    auto end() const { return mComponents.end(); }

    // 엔티티 목록 가져오기
    const std::vector<EntityID>& GetEntities() const { return mIndexToEntity; }

    // 크기 정보
    size_t Size() const { return mComponents.size(); }
    bool Empty() const { return mComponents.empty(); }

    // 메모리 정리
    void Clear() {
        mComponents.clear();
        mEntityToIndex.clear();
        mIndexToEntity.clear();
    }

private:
    std::vector<T> mComponents;                          // 실제 컴포넌트 데이터
    std::unordered_map<EntityID, size_t> mEntityToIndex; // 엔티티 -> 인덱스
    std::vector<EntityID> mIndexToEntity;                // 인덱스 -> 엔티티
};
