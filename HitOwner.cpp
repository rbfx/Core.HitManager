#include "HitOwner.h"

#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace Urho3D
{

namespace
{

bool IsExpiredHit(const ComponentHitInfo& hit)
{
    return !hit.detector_ || !hit.trigger_;
}

bool IsSameHit(const ComponentHitInfo& hit, HitDetector* detector, HitTrigger* trigger)
{
    return hit.detector_ == detector && hit.trigger_ == trigger;
}

bool IsSameHit(const GroupHitInfo& hit, HitOwner* triggerOwner,
    const ea::string& detectorGroupId, const ea::string& triggerGroupId)
{
    return hit.trigger_ == triggerOwner && hit.detectorGroup_ == detectorGroupId && hit.triggerGroup_ == triggerGroupId;
}

bool IsComponentHitActive(HitOwner* detectorOwner, HitDetector* detector, HitTrigger* trigger)
{
    return detectorOwner->IsEnabled() && trigger->IsEnabledForDetector(detector);
}

bool HasHitInCollection(ea::span<const GroupHitInfo> collection, HitOwner* triggerOwner,
    const ea::string& detectorGroupId, const ea::string& triggerGroupId)
{
    return ea::any_of(collection.begin(), collection.end(), [&](const GroupHitInfo& hit) //
    { //
        return IsSameHit(hit, triggerOwner, detectorGroupId, triggerGroupId);
    });
}

bool IsGroupMergeKeyEqual(const GroupHitInfo& lhs, const GroupHitInfo& rhs)
{
    return lhs.MergeKey() == rhs.MergeKey();
};

} // namespace

HitOwner::HitOwner(Context* context)
    : TrackedComponent<TrackedComponentBase, HitManager>(context)
{
}

void HitOwner::RegisterObject(Context* context)
{
    context->RegisterFactory<HitOwner>(Category_Plugin_HitManager);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Trigger Fade Out", GetTriggerFadeOut, SetTriggerFadeOut, float, 0.0f, AM_DEFAULT);
}

const GroupHitInfo* HitOwner::GetHitInfo(HitId id) const
{
    const auto isSameId = [id](const GroupHitInfo& hit) { return hit.id_ == id; };
    const auto iter = ea::find_if(groupHits_.begin(), groupHits_.end(), isSameId);
    return iter != groupHits_.end() ? &*iter : nullptr;
}

void HitOwner::RemoveExpiredRawHits()
{
    ea::erase_if(componentHits_, [](const ComponentHitInfo& hit) { return IsExpiredHit(hit); });
}

void HitOwner::CalculateGroupHits()
{
    ea::swap(groupHits_, previousGroupHits_);
    groupHits_.clear();

    for (const ComponentHitInfo& componentHit : componentHits_)
    {
        const bool isActive = IsComponentHitActive(this, componentHit.detector_, componentHit.trigger_);
        if (!isActive)
            continue;

        HitOwner* detectorOwner = componentHit.detector_->GetHitOwner();
        URHO3D_ASSERT(detectorOwner == this);

        HitOwner* triggerOwner = componentHit.trigger_->GetHitOwner();
        if (!triggerOwner)
        {
            URHO3D_ASSERTLOG(false, "HitOwner of the trigger is null");
            continue;
        }

        const ea::string& detectorGroupId = componentHit.detector_->GetGroupId();
        const ea::string& triggerGroupId = componentHit.trigger_->GetGroupId();
        if (HasHitInCollection(groupHits_, triggerOwner, detectorGroupId, triggerGroupId))
            continue;

        const WeakPtr<HitOwner> weakDetector{detectorOwner};
        const WeakPtr<HitOwner> weakTrigger{triggerOwner};
        groupHits_.push_back(GroupHitInfo{weakDetector, weakTrigger, detectorGroupId, triggerGroupId});
    }
}

void HitOwner::StartAndStopHits(float timeStep)
{
    for (GroupHitInfo& groupHit : groupHits_)
    {
        const auto iterPrevious =
            ea::find(previousGroupHits_.begin(), previousGroupHits_.end(), groupHit, IsGroupMergeKeyEqual);
        if (iterPrevious == previousGroupHits_.end())
        {
            groupHit.id_ = GetNextId();
            OnHitStarted(groupHit);
            continue;
        }

        URHO3D_ASSERT(iterPrevious->id_ != HitId::Invalid);
        groupHit.id_ = iterPrevious->id_;
        iterPrevious->id_ = HitId::Invalid;
    }

    for (GroupHitInfo& groupHit : previousGroupHits_)
    {
        if (groupHit.id_ == HitId::Invalid)
            continue;

        if (!groupHit.timeToExpire_)
        {
            HitOwner* triggerOwner = groupHit.trigger_;
            groupHit.timeToExpire_ = triggerOwner ? triggerOwner->GetTriggerFadeOut() : 0.0f;
        }
        else
        {
            *groupHit.timeToExpire_ -= timeStep;
        }

        if (*groupHit.timeToExpire_ <= 0.0f)
        {
            OnHitStopped(groupHit);
            continue;
        }

        // Keep expiring hit for a while
        groupHits_.push_back(groupHit);
    }
}

void HitOwner::UpdateEvents(float timeStep)
{
    RemoveExpiredRawHits();
    CalculateGroupHits();
    StartAndStopHits(timeStep);
}

void HitOwner::AddOngoingHit(HitDetector* detector, HitTrigger* trigger)
{
    for (const ComponentHitInfo& hit : componentHits_)
    {
        if (IsSameHit(hit, detector, trigger))
            return;
    }

    const WeakPtr<HitDetector> weakDetector{detector};
    const WeakPtr<HitTrigger> weakTrigger{trigger};
    componentHits_.push_back(ComponentHitInfo{weakDetector, weakTrigger});
}

void HitOwner::RemoveOngoingHit(HitDetector* detector, HitTrigger* trigger)
{
    for (ComponentHitInfo& hit : componentHits_)
    {
        if (IsSameHit(hit, detector, trigger))
        {
            hit = {};
            return;
        }
    }
}

HitId HitOwner::GetNextId()
{
    while (nextId_ == HitId::Invalid || GetHitInfo(nextId_) != nullptr)
        AdvanceNextId();

    const HitId id = nextId_;
    AdvanceNextId();
    return id;
}

void HitOwner::SendEvent(StringHash eventType, const GroupHitInfo& hit)
{
    VariantMap& eventData = GetEventDataMap();

    eventData[HitStarted::P_DETECTOR] = hit.detector_;
    eventData[HitStarted::P_DETECTOR_GROUP] = hit.detectorGroup_;
    eventData[HitStarted::P_TRIGGER] = hit.trigger_;
    eventData[HitStarted::P_TRIGGER_GROUP] = hit.triggerGroup_;
    eventData[HitStarted::P_ID] = static_cast<unsigned>(hit.id_);

    node_->SendEvent(eventType, eventData);
    GetScene()->SendEvent(eventType, eventData);
}

void HitOwner::OnHitStarted(const GroupHitInfo& hit)
{
    SendEvent(E_HITSTARTED, hit);
}

void HitOwner::OnHitStopped(const GroupHitInfo& hit)
{
    SendEvent(E_HITSTOPPED, hit);
}

HitComponent::HitComponent(Context* context)
    : LogicComponent(context)
{
}

HitComponent::~HitComponent()
{
    if (rigidBody_)
        rigidBody_->Remove();
}

void HitComponent::RegisterObject(Context* context)
{
    context->RegisterFactory<HitComponent>(Category_Plugin_HitManager);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Group Id", GetGroupId, SetGroupId, ea::string, EMPTY_STRING, AM_DEFAULT);
}

bool HitComponent::IsSelfAndOwnerEnabled()
{
    HitOwner* owner = GetHitOwner();
    return IsEnabled() && owner && owner->IsEnabled();
}

HitOwner* HitComponent::GetHitOwner()
{
    HitOwner* hitOwner = hitOwner_;
    if (hitOwner || !node_)
        return hitOwner;

    hitOwner = node_->GetComponent<HitOwner>();
    if (!hitOwner)
        hitOwner = node_->FindComponent<HitOwner>(ComponentSearchFlag::ParentRecursive);

    hitOwner_ = hitOwner;
    return hitOwner;
}

void HitComponent::DelayedStart()
{
    rigidBody_ = node_->GetComponent<RigidBody>();

    if (!rigidBody_)
    {
        rigidBody_ = node_->CreateComponent<RigidBody>();

        auto hitManager = node_->GetScene()->GetOrCreateComponent<HitManager>();
        SetupRigidBody(hitManager, rigidBody_);
    }
}

void HitTrigger::RegisterObject(Context* context)
{
    context->RegisterFactory<HitTrigger>(Category_Plugin_HitManager);

    URHO3D_COPY_BASE_ATTRIBUTES(HitComponent);
    URHO3D_ACCESSOR_ATTRIBUTE("Velocity Threshold", GetVelocityThreshold, SetVelocityThreshold, float, 0.0f, AM_DEFAULT);
}

bool HitTrigger::IsEnabledForDetector(HitDetector* hitDetector)
{
    return IsSelfAndOwnerEnabled() && (GetHitOwner() != hitDetector->GetHitOwner()) && IsVelocityThresholdSatisfied();
}

void HitTrigger::SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody)
{
    const unsigned layer = hitManager->GetTriggerCollisionLayer();
    const unsigned mask = hitManager->GetTriggerCollisionMask();

    rigidBody->SetCollisionLayerAndMask(layer, mask);
    rigidBody->SetTrigger(true);
    rigidBody->SetKinematic(true);
    rigidBody->SetMass(1.0f);
}

float HitTrigger::GetRigidBodyVelocity() const
{
    RigidBody* rigidBody = GetRigidBody();
    return rigidBody ? rigidBody->GetLinearVelocity().Length() : 0.0f;
}

bool HitTrigger::IsVelocityThresholdSatisfied() const
{
    const float velocity = GetRigidBodyVelocity();
    return velocityThreshold_ == 0.0f || velocity >= velocityThreshold_;
}

void HitDetector::RegisterObject(Context* context)
{
    context->RegisterFactory<HitDetector>(Category_Plugin_HitManager);

    URHO3D_COPY_BASE_ATTRIBUTES(HitComponent);
}

void HitDetector::DelayedStart()
{
    HitComponent::DelayedStart();

    SubscribeToEvent(node_, E_NODECOLLISIONSTART,
        [&](VariantMap& eventData)
    {
        auto otherNode = static_cast<Node*>(eventData[NodeCollisionStart::P_OTHERNODE].GetPtr());
        auto hitTrigger = otherNode->GetComponent<HitTrigger>();
        if (!hitTrigger)
            return;

        OnHitStarted(hitTrigger);
    });

    SubscribeToEvent(node_, E_NODECOLLISIONEND,
        [&](VariantMap& eventData)
    {
        auto otherNode = static_cast<Node*>(eventData[NodeCollisionEnd::P_OTHERNODE].GetPtr());
        auto hitTrigger = otherNode->GetComponent<HitTrigger>();
        if (!hitTrigger)
            return;

        OnHitStopped(hitTrigger);
    });
}

void HitDetector::SetupRigidBody(HitManager* hitManager, RigidBody* rigidBody)
{
    const unsigned layer = hitManager->GetDetectorCollisionLayer();
    const unsigned mask = hitManager->GetDetectorCollisionMask();

    rigidBody->SetCollisionLayerAndMask(layer, mask);
    rigidBody->SetKinematic(true);
    rigidBody->SetMass(1.0f);
}

void HitDetector::OnHitStarted(HitTrigger* hitTrigger)
{
    if (HitOwner* hitOwner = GetHitOwner())
    {
        if (hitOwner != hitTrigger->GetHitOwner())
            hitOwner->AddOngoingHit(this, hitTrigger);
    }
}

void HitDetector::OnHitStopped(HitTrigger* hitTrigger)
{
    if (HitOwner* hitOwner = GetHitOwner())
    {
        if (hitOwner != hitTrigger->GetHitOwner())
            hitOwner->RemoveOngoingHit(this, hitTrigger);
    }
}

} // namespace Urho3D
