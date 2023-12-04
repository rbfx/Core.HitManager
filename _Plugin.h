#pragma once

#include <Urho3D/Urho3D.h>

#if Plugin_Core_HitManager_EXPORT
    #define PLUGIN_CORE_HITMANAGER_API URHO3D_EXPORT_API
#else
    #define PLUGIN_CORE_HITMANAGER_API URHO3D_IMPORT_API
#endif
