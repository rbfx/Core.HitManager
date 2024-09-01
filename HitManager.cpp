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

    // clang-format off
    URHO3D_ATTRIBUTE("Trigger Collision Mask", unsigned, triggerCollisionMask_, DefaultTriggerCollisionMask, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Trigger Collision Layer", unsigned, triggerCollisionLayer_, DefaultTriggerCollisionLayer, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Detector Collision Mask", unsigned, detectorCollisionMask_, DefaultDetectorCollisionMask, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Detector Collision Layer", unsigned, detectorCollisionLayer_, DefaultDetectorCollisionLayer, AM_DEFAULT);
    // clang-format on
}

void HitManager::EnumerateActiveHits(ea::vector<const GroupHitInfo*>& hits)
{
    const auto owners = StaticCastSpan<HitOwner*>(GetTrackedComponents());
    for (HitOwner* owner : owners)
    {
        for (const GroupHitInfo& hit : owner->GetHits())
            hits.emplace_back(&hit);
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

void HitManager::Update(VariantMap& eventData)
{
    URHO3D_PROFILE("Update Hits");

    const float timeStep = eventData[SceneSubsystemUpdate::P_TIMESTEP].GetFloat();
    const auto owners = StaticCastSpan<HitOwner*>(GetTrackedComponents());
    for (HitOwner* owner : owners)
        owner->UpdateEvents(timeStep);
}

} // namespace Urho3D
