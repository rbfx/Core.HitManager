#pragma once

#include "HitManager.h"

#include <Urho3D/Scene/LogicComponent.h>

namespace Urho3D
{

class HitDetector;
class HitTrigger;
class RigidBody;

/// Identifier of ongoing hit. Unique within the owner.
enum class HitId : unsigned
{
};

/// Description of ongoing hit.
struct PLUGIN_CORE_HITMANAGER_API HitInfo
{
    /// Hit detector. May be null if the detector is expired.
    WeakPtr<HitDetector> detector_;
    /// Hit trigger. May be null if the trigger is expired.
    WeakPtr<HitTrigger> trigger_;
    /// Whether the hit is active. Inactive hits should be ignored.
    bool isActive_{};
    /// Id is unique within the owner.
    HitId id_{};

    bool IsExpired() const { return !detector_ || !trigger_; }
    bool IsSame(HitDetector* detector, HitTrigger* trigger) const
    {
        return detector_ == detector && trigger_ == trigger;
    }
};

class PLUGIN_CORE_HITMANAGER_API HitOwner : public TrackedComponent<TrackedComponentBase, HitManager>
{
    URHO3D_OBJECT(HitOwner, TrackedComponentBase);

public:
    HitOwner(Context* context);
    static void RegisterObject(Context* context);

    /// Return all hits, both active and inactive.
    const ea::vector<HitInfo>& GetHits() const { return hits_; }
    /// Find hit by ID.
    const HitInfo* GetHitInfo(HitId id) const;

    /// Internal.
    /// @{
    void UpdateEvents();
    void AddOngoingHit(HitDetector* detector, HitTrigger* trigger);
    void RemoveOngoingHit(HitDetector* detector, HitTrigger* trigger);
    /// @}

private:
    void AdvanceNextId() { nextId_ = static_cast<HitId>(static_cast<unsigned>(nextId_) + 1); }
    HitId GetNextId();
    void SendEvent(StringHash eventType, const HitInfo& hit);

    void OnHitStarted(const HitInfo& hit);
    void OnHitStopped(const HitInfo& hit);

    ea::vector<HitInfo> hits_;
    HitId nextId_{};
};

class PLUGIN_CORE_HITMANAGER_API HitComponent : public LogicComponent
{
    URHO3D_OBJECT(HitComponent, LogicComponent);

public:
    HitComponent(Context* context);
    ~HitComponent() override;

    HitOwner* GetHitOwner();
    bool IsSelfAndOwnerEnabled();

    /// Implement LogicComponent.
    /// @{
    void DelayedStart() override;
    /// @}

protected:
    virtual void SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody) {}

    RigidBody* GetRigidBody() const { return rigidBody_; }

private:
    WeakPtr<RigidBody> rigidBody_;
    WeakPtr<HitOwner> hitOwner_;
};

class PLUGIN_CORE_HITMANAGER_API HitTrigger : public HitComponent
{
    URHO3D_OBJECT(HitTrigger, HitComponent);

public:
    using HitComponent::HitComponent;
    static void RegisterObject(Context* context);

    bool IsEnabledForDetector(HitDetector* hitDetector);

    /// Attributes.
    /// @{
    void SetVelocityThreshold(float value) { velocityThreshold_ = value; }
    float GetVelocityThreshold() const { return velocityThreshold_; }
    /// @}

private:
    void SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody) override;

    float GetRigidBodyVelocity() const;
    bool IsVelocityThresholdSatisfied() const;

    float velocityThreshold_{};
};

class PLUGIN_CORE_HITMANAGER_API HitDetector : public HitComponent
{
    URHO3D_OBJECT(HitDetector, HitComponent);

public:
    using HitComponent::HitComponent;
    static void RegisterObject(Context* context);

    /// Implement LogicComponent.
    /// @{
    void DelayedStart() override;
    /// @}

private:
    void SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody) override;

    void OnHitStarted(HitTrigger* hitTrigger);
    void OnHitStopped(HitTrigger* hitTrigger);
};

} // namespace Urho3D
