#include "_Plugin.h"

#include "HitManager.h"
#include "HitOwner.h"

#include <Urho3D/Plugins/PluginApplication.h>

using namespace Urho3D;

namespace
{

void RegisterPluginObjects(PluginApplication& plugin)
{
    plugin.RegisterObject<HitManager>();
    plugin.RegisterObject<HitOwner>();
    plugin.RegisterObject<HitComponent>();
    plugin.RegisterObject<HitDetector>();
    plugin.RegisterObject<HitTrigger>();
}

void UnregisterPluginObjects(PluginApplication& plugin)
{
}

} // namespace

URHO3D_DEFINE_PLUGIN_MAIN_SIMPLE(RegisterPluginObjects, UnregisterPluginObjects);
