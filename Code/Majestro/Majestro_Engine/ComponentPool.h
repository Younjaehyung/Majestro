#pragma once

#include "Entity.h"

class BaseComponentPool {
public:
    virtual ~BaseComponentPool() = default;
    virtual void RemoveComponent(EntityID id) = 0;
};


class EntityIndexMap {
public:
    static constexpr uint32 INVALID_INDEX = 0xFFFFFFFFu;

 
    uint32 Find(EntityID entity) const {
        // 없으면 INVALID_INDEX
        const size_t pageIndex = entity / PAGE_SIZE;

        if (pageIndex >= mPages.size()) return INVALID_INDEX;

        const uint32* slots = mPages[pageIndex].slots.get();
        if (slots == nullptr) return INVALID_INDEX; 

        return slots[entity % PAGE_SIZE];
    }


    void Set(EntityID entity, uint32 index) {

        const size_t pageIndex = entity / PAGE_SIZE;

		if (pageIndex >= mPages.size()) // 페이지가 없으면 새로 만듦
            mPages.resize(pageIndex + 1);

        Page& page = mPages[pageIndex];
		if (page.slots == nullptr) {    // 페이지가 비어있으면 새로 할당
            page.slots = std::make_unique<uint32[]>(PAGE_SIZE);
            std::fill_n(page.slots.get(), PAGE_SIZE, INVALID_INDEX);
            page.usedCount = 0;
        }

        uint32& slot = page.slots[entity % PAGE_SIZE];
        if (slot == INVALID_INDEX)
            ++page.usedCount; 

		slot = index;   // 엔티티 ID -> 인덱스 매핑
    }

    void Erase(EntityID entity) {

        const size_t pageIndex = entity / PAGE_SIZE;

        if (pageIndex >= mPages.size()) return;

        Page& page = mPages[pageIndex];
        if (page.slots == nullptr) return;

        uint32& slot = page.slots[entity % PAGE_SIZE];
        if (slot == INVALID_INDEX) return;

        slot = INVALID_INDEX;
        if (--page.usedCount == 0)
            page.slots.reset();   // 빈 페이지는 반납해 ID 증가에 따른 누적 차단
    }

    void Clear() { mPages.clear(); }

private:
    static constexpr uint32 PAGE_SIZE = 1024;   // 페이지당 엔티티 수 (4KB)

    struct Page {
		// 캐시 효율을 위해 std::unique_ptr<uint32[]> 를 사용하여 페이지를 동적으로 할당
		std::unique_ptr<uint32[]> slots;    // 페이지 내 엔티티 ID -> 인덱스 매핑 
		uint32 usedCount = 0;               // 현재 사용 중인 슬롯 수
    };

    std::vector<Page> mPages;
};


template<typename T>
class ComponentPool : public BaseComponentPool {
public:
    ComponentPool() {
        mComponents.reserve(100);  // 초기 용량 확보
        mIndexToEntity.reserve(100);
    }

    // 컴포넌트 추가
    void AddComponent(EntityID entity, T&& component) {
        const uint32 existingIndex = mEntityToIndex.Find(entity);
        if (existingIndex != EntityIndexMap::INVALID_INDEX) {
            // 이미 존재하면 업데이트
            mComponents[existingIndex] = std::move(component);
        }
        else {
            // 새로 추가
            const uint32 index = static_cast<uint32>(mComponents.size());
            mComponents.emplace_back(std::move(component));
            mIndexToEntity.emplace_back(entity);
            mEntityToIndex.Set(entity, index);
        }
    }

    // 컴포넌트 제거
    void RemoveComponent(EntityID entity) override {
        const uint32 indexToRemove = mEntityToIndex.Find(entity);

        if (indexToRemove == EntityIndexMap::INVALID_INDEX) return;

        const uint32 lastIndex = static_cast<uint32>(mComponents.size()) - 1;
		if (indexToRemove != lastIndex) {   
			// 삭제할 요소와 마지막 요소가 다르면 마지막 요소를 삭제할 위치로 이동
            mComponents[indexToRemove] = std::move(mComponents[lastIndex]);
            const EntityID lastEntity = mIndexToEntity[lastIndex];
            mIndexToEntity[indexToRemove] = lastEntity;
            mEntityToIndex.Set(lastEntity, indexToRemove);
        }

        // 마지막 요소 제거
        mComponents.pop_back();
        mIndexToEntity.pop_back();
        mEntityToIndex.Erase(entity);
    }

    // 컴포넌트 가져오기
    T* GetComponent(EntityID entity) {
        const uint32 index = mEntityToIndex.Find(entity);
        if (index != EntityIndexMap::INVALID_INDEX) {
            return &mComponents[index];
        }
        return nullptr;
    }

    const T* GetComponent(EntityID entity) const {
        const uint32 index = mEntityToIndex.Find(entity);
        if (index != EntityIndexMap::INVALID_INDEX) {
            return &mComponents[index];
        }
        return nullptr;
    }

    // 컴포넌트 존재 여부 확인
    bool HasComponent(EntityID entity) const {
        return mEntityToIndex.Find(entity) != EntityIndexMap::INVALID_INDEX;
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
        mEntityToIndex.Clear();
        mIndexToEntity.clear();
    }

private:
    std::vector<T> mComponents;             // 실제 컴포넌트 데이터
    EntityIndexMap mEntityToIndex;          // 엔티티 -> 인덱스
    std::vector<EntityID> mIndexToEntity;   // 인덱스 -> 엔티티
};

