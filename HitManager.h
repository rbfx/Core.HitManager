#pragma once

#include "_Plugin.h"

#include <Urho3D/Scene/TrackedComponent.h>

namespace Urho3D
{

struct GroupHitInfo;
class HitDetector;
class HitOwner;
class HitTrigger;

URHO3D_EVENT(E_HITSTARTED, HitStarted)
{
    URHO3D_PARAM(P_DETECTOR, Detector); // HitOwner pointer
    URHO3D_PARAM(P_DETECTOR_GROUP, DetectorGroup); // string
    URHO3D_PARAM(P_TRIGGER, Trigger); // HitOwner pointer
    URHO3D_PARAM(P_TRIGGER_GROUP, DetectorGroup); // string
    URHO3D_PARAM(P_ID, Id); // int
}

URHO3D_EVENT(E_HITSTOPPED, HitStopped)
{
    URHO3D_PARAM(P_DETECTOR, Detector); // HitOwner pointer
    URHO3D_PARAM(P_DETECTOR_GROUP, DetectorGroup); // string
    URHO3D_PARAM(P_TRIGGER, Trigger); // HitOwner pointer
    URHO3D_PARAM(P_TRIGGER_GROUP, DetectorGroup); // string
    URHO3D_PARAM(P_ID, Id); // int
}

class PLUGIN_CORE_HITMANAGER_API HitManager : public TrackedComponentRegistryBase
{
    URHO3D_OBJECT(HitManager, TrackedComponentRegistryBase);

public:
    static constexpr unsigned DefaultTriggerCollisionLayer = 0x8000;
    static constexpr unsigned DefaultDetectorCollisionLayer = 0x4000;
    static constexpr unsigned DefaultTriggerCollisionMask = DefaultDetectorCollisionLayer;
    static constexpr unsigned DefaultDetectorCollisionMask = DefaultTriggerCollisionLayer;

    HitManager(Context* context);
    static void RegisterObject(Context* context);

    /// Enumerate all active hits happening in the scene.
    void EnumerateActiveHits(ea::vector<const GroupHitInfo*>& hits);

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
    void Update(VariantMap& eventData);

    unsigned triggerCollisionMask_{DefaultTriggerCollisionMask};
    unsigned triggerCollisionLayer_{DefaultTriggerCollisionLayer};
    unsigned detectorCollisionMask_{DefaultDetectorCollisionMask};
    unsigned detectorCollisionLayer_{DefaultDetectorCollisionLayer};
};

} // namespace Urho3D
