/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * ======================================================
 * Metamod:Source Sample Plugin
 * Written by AlliedModders LLC.
 * ======================================================
 *
 * This software is provided 'as-is', without any express or implied warranty.
 * In no event will the authors be held liable for any damages arising from
 * the use of this software.
 *
 * This sample plugin is public domain.
 */

#include "mm_plugin.h"

#include <cstdio>

#include "core/coreconfig.h"
#include "core/game_system.h"
#include "core/gameconfig.h"
#include "core/global_listener.h"
#include "core/log.h"
#include "core/managers/entity_manager.h"
#include "core/tick_scheduler.h"
#include "core/timer_system.h"
#include "core/utils.h"
#include "entity2/entitysystem.h"
#include "igameeventsystem.h"
#include "interfaces/cs2_interfaces.h"
#include "iserver.h"
#include "scripting/callback_manager.h"
#include "scripting/dotnet_host.h"
#include "scripting/script_engine.h"
#include "tier0/vprof.h"

#define VERSION_STRING  "v" SEMVER " @ " GITHUB_SHA
#define BUILD_TIMESTAMP __DATE__ " " __TIME__

counterstrikesharp::GlobalClass* counterstrikesharp::GlobalClass::head = nullptr;

CGameEntitySystem* GameEntitySystem() { return counterstrikesharp::globals::entitySystem; }

// TODO: Workaround for windows, we __MUST__ have COUNTERSTRIKESHARP_API to handle it.
// like on windows it should be `extern "C" __declspec(dllexport)`, on linux it should be anything else.
DLL_EXPORT void InvokeNative(counterstrikesharp::fxNativeContext& context)
{
    if (context.nativeIdentifier == 0) return;

    if (context.nativeIdentifier != counterstrikesharp::hash_string_const("QUEUE_TASK_FOR_NEXT_FRAME") &&
        context.nativeIdentifier != counterstrikesharp::hash_string_const("QUEUE_TASK_FOR_NEXT_WORLD_UPDATE") &&
        context.nativeIdentifier != counterstrikesharp::hash_string_const("QUEUE_TASK_FOR_FRAME") &&
        counterstrikesharp::globals::gameThreadId != std::this_thread::get_id())
    {
        counterstrikesharp::ScriptContextRaw scriptContext(context);
        scriptContext.ThrowNativeError("Invoked on a non-main thread");

        CSSHARP_CORE_CRITICAL("Native {:x} was invoked on a non-main thread", context.nativeIdentifier);
        return;
    }

    counterstrikesharp::ScriptEngine::InvokeNative(context);
}

class GameSessionConfiguration_t
{
};

namespace counterstrikesharp {

SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);
SH_DECL_HOOK3_void(
    INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);
SH_DECL_HOOK3_void(IEngineServiceMgr, RegisterLoopMode, SH_NOATTRIB, 0, const char*, ILoopModeFactory*, void**);
SH_DECL_HOOK1(IEngineServiceMgr, FindService, SH_NOATTRIB, 0, IEngineService*, const char*);

CounterStrikeSharpMMPlugin plugin;

#if 0
// Currently unavailable, requires hl2sdk work!
ConVar sample_cvar("sample_cvar", "42", 0);
#endif

PLUGIN_EXPOSE(CounterStrikeSharpMMPlugin, plugin);
bool CounterStrikeSharpMMPlugin::Load(const PluginId id, ISmmAPI* ismm, char* error, const size_t maxlen, bool late)
{
    PLUGIN_SAVEVARS();
    globals::ismm = ismm;
    globals::gameThreadId = std::this_thread::get_id();

    Log::Init();

    CSSHARP_CORE_INFO("Initializing");

    GET_V_IFACE_CURRENT(GetEngineFactory, globals::engineServer2, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, globals::engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
    GET_V_IFACE_CURRENT(GetEngineFactory, globals::cvars, ICvar, CVAR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, globals::server, IServerGameDLL, INTERFACEVERSION_SERVERGAMEDLL);
    GET_V_IFACE_ANY(GetServerFactory, globals::serverGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    GET_V_IFACE_ANY(GetEngineFactory, globals::networkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, globals::schemaSystem, CSchemaSystem, SCHEMASYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, globals::gameEventSystem, IGameEventSystem, GAMEEVENTSYSTEM_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, globals::engineServiceManager, IEngineServiceMgr, ENGINESERVICEMGR_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetEngineFactory, globals::networkMessages, INetworkMessages, NETWORKMESSAGES_INTERFACE_VERSION);
    GET_V_IFACE_ANY(GetServerFactory, globals::gameEntities, ISource2GameEntities, SOURCE2GAMEENTITIES_INTERFACE_VERSION);

    auto CoreconfigPath = std::string(utils::ConfigsDirectory() + "/core");
    globals::coreConfig = new CCoreConfig(CoreconfigPath);

    if (char szCoreConfigError[255] = ""; !globals::coreConfig->Init(szCoreConfigError, sizeof(szCoreConfigError)))
    {
        CSSHARP_CORE_ERROR("Could not read \'{}\'. Error: {}", CoreconfigPath, szCoreConfigError);
        return false;
    }

    CSSHARP_CORE_INFO("CoreConfig loaded.");

    auto gamedata_path = std::string(utils::GamedataDirectory() + "/gamedata.json");
    globals::gameConfig = new CGameConfig(gamedata_path);

    if (char conf_error[255] = ""; !globals::gameConfig->Init(conf_error, sizeof(conf_error)))
    {
        CSSHARP_CORE_ERROR("Could not read \'{}\'. Error: {}", gamedata_path, conf_error);
        return false;
    }

    globals::Initialize();

    CSSHARP_CORE_INFO("Globals loaded.");
    globals::mmPlugin = &plugin;

    CALL_GLOBAL_LISTENER(OnAllInitialized());

    globals::onActivateCallback = globals::callbackManager.CreateCallback("OnMapStart");
    globals::onMetamodAllPluginsLoaded = globals::callbackManager.CreateCallback("OnMetamodAllPluginsLoaded");

    SH_ADD_HOOK_MEMFUNC(IServerGameDLL, GameFrame, globals::server, this, &CounterStrikeSharpMMPlugin::HookGameFrame, true);
    SH_ADD_HOOK_STATICFUNC(INetworkServerService, StartupServer, globals::networkServerService,
                           &CounterStrikeSharpMMPlugin::HookStartupServer, true);
    SH_ADD_HOOK_STATICFUNC(IEngineServiceMgr, RegisterLoopMode, globals::engineServiceManager,
                           &CounterStrikeSharpMMPlugin::HookRegisterLoopMode, false);
    SH_ADD_HOOK_STATICFUNC(IEngineServiceMgr, FindService, globals::engineServiceManager, &CounterStrikeSharpMMPlugin::HookFindService,
                           true);

    if (!globals::dotnetManager.Initialize())
    {
        CSSHARP_CORE_ERROR("Failed to initialize .NET runtime");
    }

    if (!InitGameSystems())
    {
        CSSHARP_CORE_ERROR("Failed to initialize GameSystem!");
        return false;
    }
    CSSHARP_CORE_INFO("Initialized GameSystem.");

    CSSHARP_CORE_INFO("Hooks added.");

    // Used by Metamod Console Commands
    g_pCVar = globals::cvars;
    ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

    return true;
}

void CounterStrikeSharpMMPlugin::HookStartupServer(const GameSessionConfiguration_t&, ISource2WorldSession*, const char*)
{
    globals::entitySystem = interfaces::pGameResourceServiceServer->GetGameEntitySystem();
    globals::entitySystem->AddListenerEntity(&globals::entityManager.entityListener);
    globals::timerSystem.OnStartupServer();

    globals::onActivateCallback->ScriptContext().Reset();
    globals::onActivateCallback->ScriptContext().Push(globals::getGlobalVars()->mapname.ToCStr());
    globals::onActivateCallback->Execute();
}

bool CounterStrikeSharpMMPlugin::Unload(char* error, size_t maxlen)
{
    SH_REMOVE_HOOK_MEMFUNC(IServerGameDLL, GameFrame, globals::server, this, &CounterStrikeSharpMMPlugin::HookGameFrame, true);
    SH_REMOVE_HOOK_STATICFUNC(INetworkServerService, StartupServer, globals::networkServerService,
                              &CounterStrikeSharpMMPlugin::HookStartupServer, true);

    globals::callbackManager.ReleaseCallback(globals::onActivateCallback);
    globals::callbackManager.ReleaseCallback(globals::onMetamodAllPluginsLoaded);

    return true;
}

void CounterStrikeSharpMMPlugin::AllPluginsLoaded()
{
    /* This is where we'd do stuff that relies on the mod or other plugins
     * being initialized (for example, cvars added and events registered).
     */
    globals::onMetamodAllPluginsLoaded->ScriptContext().Reset();
    globals::onMetamodAllPluginsLoaded->Execute();
}

void CounterStrikeSharpMMPlugin::AddTaskForNextFrame(std::function<void()>&& Task)
{
    m_nextTasks.try_enqueue(std::forward<decltype(Task)>(Task));
}

void CounterStrikeSharpMMPlugin::HookGameFrame(const bool bSimulating, bool, bool)
{
    /**
     * simulating:
     * ***********
     * true  | game is ticking
     * false | game is not ticking
     */
    VPROF_BUDGET("CS#::Hook_GameFrame", "CS# On Frame");
    globals::timerSystem.OnGameFrame(bSimulating);

    std::vector<std::function<void()>> out_list(1024);

    if (auto size = m_nextTasks.try_dequeue_bulk(out_list.begin(), 1024); size > 0)
    {
        CSSHARP_CORE_TRACE("Executing queued tasks of size: {0} on tick number {1}", size, globals::getGlobalVars()->tickcount);

        for (size_t i = 0; i < size; i++)
        {
            out_list[i]();
        }
    }

    if (auto callbacks = globals::tickScheduler.getCallbacks(globals::getGlobalVars()->tickcount); !callbacks.empty())
    {
        CSSHARP_CORE_TRACE("Executing frame specific tasks of size: {0} on tick number {1}", callbacks.size(),
                           globals::getGlobalVars()->tickcount);

        for (auto& callback : callbacks)
        {
            callback();
        }
    }
}

// Potentially might not work
void CounterStrikeSharpMMPlugin::OnLevelInit(
    char const* pMapName, char const* pMapEntities, char const* pOldLevel, char const* pLandmarkName, bool loadGame, bool background)
{
    CSSHARP_CORE_TRACE("name={0},mapname={1}", "LevelInit", pMapName);
}

void CounterStrikeSharpMMPlugin::HookRegisterLoopMode(const char* szLoopModeName, ILoopModeFactory*, void**)
{
    if (strcmp(szLoopModeName, "game") == 0)
    {
        if (!globals::gameLoopInitialized) globals::gameLoopInitialized = true;

        CALL_GLOBAL_LISTENER(OnGameLoopInitialized());
    }
}

IEngineService* CounterStrikeSharpMMPlugin::HookFindService(const char*)
{
    IEngineService* pService = META_RESULT_ORIG_RET(IEngineService*);

    return pService;
}

void CounterStrikeSharpMMPlugin::OnLevelShutdown() {}

bool CounterStrikeSharpMMPlugin::Pause(char* error, size_t maxlen) { return true; }

bool CounterStrikeSharpMMPlugin::Unpause(char* error, size_t maxlen) { return true; }

const char* CounterStrikeSharpMMPlugin::GetLicense() { return "GNU GPLv3"; }

const char* CounterStrikeSharpMMPlugin::GetVersion() { return VERSION_STRING; }

const char* CounterStrikeSharpMMPlugin::GetDate() { return BUILD_TIMESTAMP; }

const char* CounterStrikeSharpMMPlugin::GetLogTag() { return "CSSHARP"; }

const char* CounterStrikeSharpMMPlugin::GetAuthor() { return "Roflmuffin"; }

const char* CounterStrikeSharpMMPlugin::GetDescription() { return "Counter Strike .NET Scripting Runtime"; }

const char* CounterStrikeSharpMMPlugin::GetName() { return "CounterStrikeSharp"; }

const char* CounterStrikeSharpMMPlugin::GetURL() { return "https://github.com/roflmuffin/CounterStrikeSharp"; }
} // namespace counterstrikesharp
