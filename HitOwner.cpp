#include "HitOwner.h"

#include <Urho3D/IO/Log.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

namespace Urho3D
{

HitOwner::HitOwner(Context* context)
    : TrackedComponent<TrackedComponentBase, HitManager>(context)
{
}

void HitOwner::RegisterObject(Context* context)
{
    context->RegisterFactory<HitOwner>(Category_User);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
}

const HitInfo* HitOwner::GetHitInfo(HitId id) const
{
    const auto iter = ea::find_if(hits_.begin(), hits_.end(), [id](const HitInfo& hit) { return hit.id_ == id; });
    return iter != hits_.end() ? &*iter : nullptr;
}

void HitOwner::UpdateEvents()
{
    for (HitInfo& ongoingHit : hits_)
    {
        const bool isActive = IsEnabled() && ongoingHit.trigger_ && ongoingHit.detector_
            && ongoingHit.trigger_->IsEnabledForDetector(ongoingHit.detector_);
        if (ongoingHit.isActive_ != isActive)
        {
            ongoingHit.isActive_ = isActive;
            if (isActive)
                OnHitStarted(ongoingHit);
            else
                OnHitStopped(ongoingHit);
        }
    }

    ea::erase_if(hits_, [](const HitInfo& ongoingHit) { return ongoingHit.IsExpired(); });
}

void HitOwner::AddOngoingHit(HitDetector* detector, HitTrigger* trigger)
{
    const WeakPtr<HitDetector> weakDetector{detector};
    const WeakPtr<HitTrigger> weakTrigger{trigger};
    const bool isActive = IsEnabled() && trigger->IsEnabledForDetector(detector);
    hits_.emplace_back(HitInfo{weakDetector, weakTrigger, isActive, GetNextId()});
    if (isActive)
        OnHitStarted(hits_.back());
}

void HitOwner::RemoveOngoingHit(HitDetector* detector, HitTrigger* trigger)
{
    const auto checkAndRemoveHit = [&](const HitInfo& ongoingHit)
    {
        if (ongoingHit.IsSame(detector, trigger))
        {
            if (ongoingHit.isActive_)
                OnHitStopped(ongoingHit);
            return true;
        }
        return false;
    };

    ea::erase_if(hits_, checkAndRemoveHit);
}

HitId HitOwner::GetNextId()
{
    while (GetHitInfo(nextId_) != nullptr)
        AdvanceNextId();

    const HitId id = nextId_;
    AdvanceNextId();
    return id;
}

void HitOwner::SendEvent(StringHash eventType, const HitInfo& hit)
{
    VariantMap& eventData = GetEventDataMap();

    eventData[HitStarted::P_RECEIVER] = this;
    eventData[HitStarted::P_HITDETECTOR] = hit.detector_;
    eventData[HitStarted::P_HITTRIGGER] = hit.trigger_;
    eventData[HitStarted::P_ID] = static_cast<unsigned>(hit.id_);

    node_->SendEvent(eventType, eventData);
    GetScene()->SendEvent(eventType, eventData);
}

void HitOwner::OnHitStarted(const HitInfo& hit)
{
    SendEvent(E_HITSTARTED, hit);
}

void HitOwner::OnHitStopped(const HitInfo& hit)
{
    SendEvent(E_HITSTOPPED, hit);
}

HitComponent::HitComponent(Context* context)
    : Component(context)
{
}

HitComponent::~HitComponent()
{
    if (rigidBody_)
        rigidBody_->Remove();
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
        hitOwner = node_->GetParentComponent<HitOwner>(true);

    hitOwner_ = hitOwner;
    return hitOwner;
}

void HitComponent::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (!node_ || !node_->GetScene())
    {
        rigidBody_ = nullptr;
    }
    else
    {
        auto hitManager = node_->GetScene()->GetOrCreateComponent<HitManager>();
        const unsigned collisionMask = hitManager->GetCollisionMask();

        auto rigidBody = node_->GetOrCreateComponent<RigidBody>();
        rigidBody->SetTemporary(true);
        rigidBody->SetCollisionLayerAndMask(collisionMask, collisionMask);

        rigidBody_ = rigidBody;
        SetupRigidBody(rigidBody);
    }
}

void HitTrigger::RegisterObject(Context* context)
{
    context->RegisterFactory<HitTrigger>(Category_User);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Velocity Threshold", GetVelocityThreshold, SetVelocityThreshold, float, 0.0f, AM_DEFAULT);
}

bool HitTrigger::IsEnabledForDetector(HitDetector* hitDetector)
{
    return IsSelfAndOwnerEnabled() && (GetHitOwner() != hitDetector->GetHitOwner()) && IsVelocityThresholdSatisfied();
}

void HitTrigger::SetupRigidBody(RigidBody* rigidBody)
{
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
    context->RegisterFactory<HitDetector>(Category_User);
}

void HitDetector::SetupRigidBody(RigidBody* rigidBody)
{
    rigidBody->SetKinematic(true);
    rigidBody->SetMass(1.0f);

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

void HitDetector::OnHitStarted(HitTrigger* hitTrigger)
{
    if (HitOwner* hitOwner = GetHitOwner())
        hitOwner->AddOngoingHit(this, hitTrigger);
}

void HitDetector::OnHitStopped(HitTrigger* hitTrigger)
{
    if (HitOwner* hitOwner = GetHitOwner())
        hitOwner->RemoveOngoingHit(this, hitTrigger);
}

} // namespace Urho3D
