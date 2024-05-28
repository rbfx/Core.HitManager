#pragma once

#include "_Plugin.h"

#include <Urho3D/Scene/TrackedComponent.h>

namespace Urho3D
{

struct HitInfo;
class HitDetector;
class HitOwner;
class HitTrigger;

URHO3D_EVENT(E_HITSTARTED, HitStarted)
{
    URHO3D_PARAM(P_RECEIVER, Receiver); // HitOwner pointer
    URHO3D_PARAM(P_HITDETECTOR, HitDetector); // HitDetector pointer
    URHO3D_PARAM(P_HITTRIGGER, HitTrigger); // HitTrigger pointer
    URHO3D_PARAM(P_ID, Id); // int
}

URHO3D_EVENT(E_HITSTOPPED, HitStopped)
{
    URHO3D_PARAM(P_RECEIVER, Receiver); // HitOwner pointer
    URHO3D_PARAM(P_HITDETECTOR, HitDetector); // HitDetector pointer
    URHO3D_PARAM(P_HITTRIGGER, HitTrigger); // HitTrigger pointer
    URHO3D_PARAM(P_ID, Id); // int
}

class PLUGIN_CORE_HITMANAGER_API HitManager : public TrackedComponentRegistryBase
{
    URHO3D_OBJECT(HitManager, TrackedComponentRegistryBase);

public:
    static constexpr unsigned DefaultCollisionMask = 0x7000;
    static constexpr unsigned DefaultCollisionLayer = 0x7000;

    HitManager(Context* context);
    static void RegisterObject(Context* context);

    /// Enumerate all active hits happening in the scene.
    void EnumerateActiveHits(ea::vector<ea::pair<HitOwner*, const HitInfo*>>& hits);

    /// Attributes.
    /// @{
    void SetTriggerCollisionMask(unsigned collisionMask) { triggerCollisionMask_ = collisionMask; }
    unsigned GetTriggerCollisionMask() const { return triggerCollisionMask_; }
    void SetTriggerCollisionLayer(unsigned collisionLayer) { triggerCollisionLayer_ = collisionLayer; }
    unsigned GetTriggerCollisionLayer() const { return triggerCollisionLayer_; }
    void SetDetectorCollisionMask(unsigned collisionMask) { detectorCollisionMask_ = collisionMask; }
    unsigned GetDetectorCollisionMask() const { return detectorCollisionMask_; }
    void SetDetectorCollisionLayer(unsigned collisionLayer) { detectorCollisionLayer_ = collisionLayer; }
    unsigned GetDetectorCollisionLayer() const { return detectorCollisionLayer_; }
    /// @}

protected:
    /// Implement TrackedComponentRegistryBase.
    /// @{
    void OnAddedToScene(Scene* scene) override;
    void OnRemovedFromScene() override;
    /// @}

private:
    void Update();

    unsigned triggerCollisionMask_{DefaultCollisionMask};
    unsigned triggerCollisionLayer_{DefaultCollisionLayer};
    unsigned detectorCollisionMask_{DefaultCollisionMask};
    unsigned detectorCollisionLayer_{DefaultCollisionLayer};
};

} // namespace Urho3D
