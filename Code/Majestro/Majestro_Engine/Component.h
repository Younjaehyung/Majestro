#pragma once
#include "Object.h"

using ComponentTypeID = uint32;


ComponentTypeID AllocateComponentTypeID();



class BaseComponent{
public:
    BaseComponent();
    virtual ~BaseComponent() = default;
    virtual std::unique_ptr<BaseComponent> Clone() const = 0;

};

template<typename T>
class Component : public BaseComponent {
public:
    std::unique_ptr<BaseComponent> Clone() const override {
        return std::make_unique<T>(static_cast<const T&>(*this));
    }

    static ComponentTypeID GetTypeID() {
        static const ComponentTypeID sTypeID = AllocateComponentTypeID();
        return sTypeID;
    }
public:
	bool			mIsActive = true;
	bool			mIsDirty = false;
	bool			mReplicateFlags = false;
};
