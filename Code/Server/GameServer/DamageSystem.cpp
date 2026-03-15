#include "pch.h"
#include "DamageSystem.h"
#include "World.h"
#include "EventManager.h"
#include "GameEvents.h"
#include "HealthComponent.h"
#include "ArmorComponent.h"

DamageSystem::DamageSystem(World* world)
    : System(world)
{
}

void DamageSystem::Update(float deltaTime)
{
    (void)deltaTime;

    auto eventManager = mWorld->GetEventManager();
    if (!eventManager)
        return;

    eventManager->Consume<EvDamage>([&](const EvDamage& e)
        {
            if (!e.target.IsValid())
                return;

            HealthComponent* health = mWorld->GetComponent<HealthComponent>(e.target);
            if (!health)
                return;

            
            const int32 appliedDamage = (std::max)(0, e.amount);
            const int32 beforeHp = health->mCurrentHp;

            int32 remainDamage = appliedDamage;
            int32 beforeArmor = 0;
            int32 afterArmor = 0;
            ArmorComponent* armor = mWorld->GetComponent<ArmorComponent>(e.target);
            if (armor)
            {
                beforeArmor = armor->mCurrentArmor;
                armor->mCurrentArmor = (std::max)(0, armor->mCurrentArmor - remainDamage);
                afterArmor = armor->mCurrentArmor;
                remainDamage = (std::max)(0, remainDamage - beforeArmor);

                if (beforeArmor != afterArmor)
                {
                    EvArmorChanged armorChanged{};
                    armorChanged.target = e.target;
                    armorChanged.currentArmor = afterArmor;
                    armorChanged.maxArmor = armor->mMaxArmor;
                    eventManager->Enqueue<EvArmorChanged>(armorChanged);
                }
            }

            health->mCurrentHp = (std::max)(0, health->mCurrentHp - remainDamage);
            const int32 afterHp = health->mCurrentHp;

            if (beforeHp != afterHp)
            {
                EvHealthChanged healthChanged{};
                healthChanged.target = e.target;
                healthChanged.currentHp = afterHp;
                healthChanged.maxHp = health->mMaxHp;
                eventManager->Enqueue<EvHealthChanged>(healthChanged);
            }

            std::cout << "[DamageSystem] target=" << e.target.GetID()
                << " instigator=" << e.instigator.GetID()
                << " amount=" << e.amount
                << " armor=" << beforeArmor << "->" << afterArmor
                << " hp=" << afterHp << "/" << health->mMaxHp
                << " (" << beforeHp << "->" << afterHp << ")" << std::endl;

            if (health->IsDead())
            {
                EvDespawn despawnEvent{};
                despawnEvent.target = e.target;
                eventManager->Enqueue<EvDespawn>(despawnEvent);
            }
        });

    eventManager->Consume<EvDespawn>([&](const EvDespawn& e)
        {
            if (!e.target.IsValid())
                return;

            //mWorld->DestroyEntity(e.target);
        });
}