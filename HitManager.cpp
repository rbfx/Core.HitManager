#include "HitManager.h"

#include "HitOwner.h"

#include <Urho3D/Container/TransformedSpan.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace Urho3D
{

HitManager::HitManager(Context* context)
    : TrackedComponentRegistryBase(context, HitOwner::GetTypeStatic())
{
}

void HitManager::RegisterObject(Context* context)
{
    context->RegisterFactory<HitManager>(Category_User);

    URHO3D_ATTRIBUTE("Trigger Collision Mask", unsigned, triggerCollisionMask_, DefaultCollisionMask, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Trigger Collision Layer", unsigned, triggerCollisionLayer_, DefaultCollisionLayer, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Detector Collision Mask", unsigned, detectorCollisionMask_, DefaultCollisionMask, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Detector Collision Layer", unsigned, detectorCollisionLayer_, DefaultCollisionLayer, AM_DEFAULT);
}

void HitManager::EnumerateActiveHits(ea::vector<ea::pair<HitOwner*, const HitInfo*>>& hits)
{
    const auto owners = StaticCastSpan<HitOwner*>(GetTrackedComponents());
    for (HitOwner* owner : owners)
    {
        for (const HitInfo& hit : owner->GetHits())
        {
            if (hit.isActive_)
                hits.emplace_back(owner, &hit);
        }
    }
}

void HitManager::OnAddedToScene(Scene* scene)
{
    SubscribeToEvent(scene, E_SCENESUBSYSTEMUPDATE, &HitManager::Update);
}

void HitManager::OnRemovedFromScene()
{
    UnsubscribeFromEvent(E_SCENESUBSYSTEMUPDATE);
}

void HitManager::Update()
{
    const auto owners = StaticCastSpan<HitOwner*>(GetTrackedComponents());
    for (HitOwner* owner : owners)
        owner->UpdateEvents();
}

} // namespace Urho3D
