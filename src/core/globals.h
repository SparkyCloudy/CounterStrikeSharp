#pragma once

#define protected public
#define private   public
#include <tier1/convar.h>
#undef protected
#undef private

#include <sourcehook/sourcehook.h>

#include <memory>
#include <thread>

#include "ISmmAPI.h"
#include "eiface.h"
#include "iserver.h"

class IGameEventManager2;
class IPlayerInfoManager;
class IBotManager;
class IServerPluginHelpers;
class IUniformRandomStream;
class IEngineTrace;
class IEngineSound;
class IEngineServiceMgr;
class INetworkStringTableContainer;
class CGlobalVars;
class IFileSystem;
class INetworkMessages;
class IServerTools;
class IPhysics;
class IPhysicsCollision;
class IPhysicsSurfaceProps;
class IMDLCache;
class IVoiceServer;
class CGlobalEntityList;
class CDotNetManager;
class ICvar;
class IGameEventSystem;
class CounterStrikeSharpMMPlugin;
class CGameEntitySystem;
class IGameEventListener2;
class CSchemaSystem;

namespace counterstrikesharp {
class EntityListener;
class EventManager;
class UserMessageManager;
class ConCommandManager;
class CallbackManager;
class ConVarManager;
class PlayerManager;
class MenuManager;
class TickScheduler;
class TimerSystem;
class ChatCommands;
class HookManager;
class EntityManager;
class ChatManager;
class ServerManager;
class VoiceManager;
class CCoreConfig;
class CGameConfig;
class ScriptCallback;
class CounterStrikeSharpMMPlugin;

namespace globals {

extern IVEngineServer* engine;
extern IVEngineServer2* engineServer2;
extern IGameEventManager2* gameEventManager;
extern IPlayerInfoManager* playerinfoManager;
extern IBotManager* botManager;
extern IServerPluginHelpers* helpers;
extern IUniformRandomStream* randomStream;
extern IEngineTrace* engineTrace;
extern IEngineSound* engineSound;
extern IEngineServiceMgr* engineServiceManager;
extern INetworkMessages* networkMessages;
extern INetworkStringTableContainer* netStringTables;
extern CGlobalVars* globalVars;
extern IFileSystem* fileSystem;
extern IServerGameDLL* serverGameDll;
extern IServerGameClients* serverGameClients;
extern INetworkServerService* networkServerService;
extern CSchemaSystem* schemaSystem;
extern IServerTools* serverTools;
extern IPhysics* physics;
extern IPhysicsCollision* physicsCollision;
extern IPhysicsSurfaceProps* physicsSurfaceProps;
extern IMDLCache* modelCache;
extern IVoiceServer* voiceServer;
extern CDotNetManager dotnetManager;
extern ICvar* cvars;
extern ISource2Server* server;
extern CGlobalEntityList* globalEntityList;
extern EntityListener entityListener;
extern CGameEntitySystem* entitySystem;
extern ISource2GameEntities* gameEntities;

extern EventManager eventManager;
extern UserMessageManager userMessageManager;
extern ConCommandManager conCommandManager;
extern CallbackManager callbackManager;
extern ConVarManager convarManager;
extern PlayerManager playerManager;
extern MenuManager menuManager;
extern EntityManager entityManager;
extern TimerSystem timerSystem;
extern ChatCommands chatCommands;
extern ChatManager chatManager;
extern ServerManager serverManager;
extern VoiceManager voiceManager;
extern TickScheduler tickScheduler;

extern HookManager hookManager;
extern SourceHook::ISourceHook* source_hook;
extern PluginId source_hook_pluginid;
extern IGameEventSystem* gameEventSystem;
extern CounterStrikeSharpMMPlugin* mmPlugin;
extern ISmmAPI* ismm;
extern ISmmPlugin* plApi;
extern CCoreConfig* coreConfig;
extern CGameConfig* gameConfig;

extern ScriptCallback* onActivateCallback;
extern ScriptCallback* onMetamodAllPluginsLoaded;
extern CounterStrikeSharpMMPlugin plugin;

extern const float engine_fixed_tick_interval;

typedef void GameEventManagerInit_t(IGameEventManager2* gameEventManager);
typedef IGameEventListener2* GetLegacyGameEventListener_t(CPlayerSlot slot);

static void DetourGameEventManagerInit(IGameEventManager2* pGameEventManager);

extern bool gameLoopInitialized;
extern GetLegacyGameEventListener_t* GetLegacyGameEventListener;
extern std::thread::id gameThreadId;

void Initialize();
// Should only be called within the active game loop (i e map should be loaded
// and active) otherwise that'll be nullptr!
CGlobalVars* getGlobalVars();
} // namespace globals

namespace modules {
class CModule;

void Initialize();
CModule* GetModuleByName(std::string name);

extern std::vector<std::unique_ptr<CModule>> moduleList;

extern CModule* engine;
extern CModule* tier0;
extern CModule* server;
extern CModule* schemasystem;
extern CModule* vscript;
} // namespace modules

} // namespace counterstrikesharp

#undef SH_GLOB_SHPTR
#define SH_GLOB_SHPTR counterstrikesharp::globals::source_hook
#undef SH_GLOB_PLUGPTR
#define SH_GLOB_PLUGPTR counterstrikesharp::globals::source_hook_pluginid

#undef SH_DECL_HOOK3_void
#define SH_DECL_HOOK3_void(ifacetype, ifacefunc, attr, overload, param1, param2, param3)                                               \
    SHINT_MAKE_GENERICSTUFF_BEGIN(ifacetype, ifacefunc, overload,                                                                      \
                                  (static_cast<void (ifacetype::*)(param1, param2, param3) attr>(&ifacetype::ifacefunc)))              \
    typedef fastdelegate::FastDelegate<void, param1, param2, param3> FD;                                                               \
    MAKE_DELEG_void((param1 p1, param2 p2, param3 p3), (p1, p2, p3));                                                                  \
    virtual void Func(param1 p1, param2 p2, param3 p3) { SH_HANDLEFUNC_void((param1, param2, param3), (p1, p2, p3)); }                 \
    SHINT_MAKE_GENERICSTUFF_END(ifacetype, ifacefunc, overload,                                                                        \
                                (static_cast<void (ifacetype::*)(param1, param2, param3) attr>(&ifacetype::ifacefunc)))                \
                                                                                                                                       \
    const ::SourceHook::PassInfo __SourceHook_ParamInfos_##ifacetype##ifacefunc##overload[] = {                                        \
        { 1, 0, 0 }, __SH_GPI(param1), __SH_GPI(param2), __SH_GPI(param3)                                                              \
    };                                                                                                                                 \
    const ::SourceHook::PassInfo::V2Info __SourceHook_ParamInfos2_##ifacetype##ifacefunc##overload[] = { __SH_EPI, __SH_EPI, __SH_EPI, \
                                                                                                         __SH_EPI };                   \
    ::SourceHook::ProtoInfo SH_FHCls(ifacetype, ifacefunc,                                                                             \
                                     overload)::ms_Proto = { 3, { 0, 0, 0 }, __SourceHook_ParamInfos_##ifacetype##ifacefunc##overload, \
                                                             0, __SH_EPI,    __SourceHook_ParamInfos2_##ifacetype##ifacefunc##overload };
