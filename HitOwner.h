#pragma once

#include "HitManager.h"

#include <Urho3D/Scene/LogicComponent.h>

#include <EASTL/tuple.h>

namespace Urho3D
{

class HitDetector;
class HitOwner;
class HitTrigger;
class RigidBody;

/// Identifier of ongoing hit. Unique within the instance of HitOwner.
enum class HitId : unsigned
{
    Invalid
};

/// Description of ongoing physical volume hit between HitTrigger and HitDetector.
/// Components that belong to the same HitOwner never hit each other.
/// There is no other filtering at this level.
struct PLUGIN_CORE_HITMANAGER_API ComponentHitInfo
{
    WeakPtr<HitDetector> detector_;
    WeakPtr<HitTrigger> trigger_;
};

/// Description of logical hit between two HitOwner objects.
struct PLUGIN_CORE_HITMANAGER_API GroupHitInfo
{
    WeakPtr<HitOwner> detector_;
    WeakPtr<HitOwner> trigger_;
    ea::string detectorGroup_;
    ea::string triggerGroup_;
    HitId id_{};
    /// Time before already stopped hit expires.
    ea::optional<float> timeToExpire_;

    /// Merge key is used to compare and merge sets of hits from different frames.
    /// Hits that belong to the same HitOwner are supposed to have unique key triplets.
    auto MergeKey() const { return ea::tie(trigger_, detectorGroup_, triggerGroup_); }
};

class PLUGIN_CORE_HITMANAGER_API HitOwner : public TrackedComponent<TrackedComponentBase, HitManager>
{
    URHO3D_OBJECT(HitOwner, TrackedComponentBase);

public:
    HitOwner(Context* context);
    static void RegisterObject(Context* context);

    /// Return all hits.
    const ea::vector<GroupHitInfo>& GetHits() const { return groupHits_; }
    /// Find hit by ID.
    const GroupHitInfo* GetHitInfo(HitId id) const;

    /// Attributes.
    /// @{
    void SetTriggerFadeOut(float value) { triggerFadeOut_ = value; }
    float GetTriggerFadeOut() const { return triggerFadeOut_; }
    /// @}

    /// Internal.
    /// @{
    void UpdateEvents(float timeStep);
    void AddOngoingHit(HitDetector* detector, HitTrigger* trigger);
    void RemoveOngoingHit(HitDetector* detector, HitTrigger* trigger);
    /// @}

private:
    void RemoveExpiredRawHits();
    void CalculateGroupHits();
    void StartAndStopHits(float timeStep);

    void AdvanceNextId() { nextId_ = static_cast<HitId>(static_cast<unsigned>(nextId_) + 1); }
    HitId GetNextId();

    void SendEvent(StringHash eventType, const GroupHitInfo& hit);
    void OnHitStarted(const GroupHitInfo& hit);
    void OnHitStopped(const GroupHitInfo& hit);

    ea::vector<ComponentHitInfo> componentHits_;
    ea::vector<GroupHitInfo> groupHits_;
    ea::vector<GroupHitInfo> previousGroupHits_;

    HitId nextId_{};

    float triggerFadeOut_{};
};

class PLUGIN_CORE_HITMANAGER_API HitComponent : public LogicComponent
{
    URHO3D_OBJECT(HitComponent, LogicComponent);

public:
    HitComponent(Context* context);
    ~HitComponent() override;
    static void RegisterObject(Context* context);

    HitOwner* GetHitOwner();
    bool IsSelfAndOwnerEnabled();

    /// Implement LogicComponent.
    /// @{
    void DelayedStart() override;
    /// @}

    /// Attributes.
    /// @{
    void SetGroupId(const ea::string& value) { groupId_ = value; }
    const ea::string& GetGroupId() const { return groupId_; }
    /// @}

protected:
    virtual void SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody) {}

    RigidBody* GetRigidBody() const { return rigidBody_; }

private:
    WeakPtr<RigidBody> rigidBody_;
    WeakPtr<HitOwner> hitOwner_;

    ea::string groupId_;
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
