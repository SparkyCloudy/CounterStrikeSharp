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

#pragma once
#define VPROF_LEVEL 1

// clang-format off
#include <ISmmPlugin.h>
#include <functional>
#include <iserver.h>
#include <igameevents.h>
#include <iplayerinfo.h>
#include <sh_vector.h>
#include <vector>
#include "entitysystem.h"
#include "concurrentqueue.h"
// clang-format on

namespace counterstrikesharp {
class CounterStrikeSharpMMPlugin final : public ISmmPlugin, public IMetamodListener
{
  public:
    bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late) override;
    bool Unload(char* error, size_t maxlen) override;
    bool Pause(char* error, size_t maxlen) override;
    bool Unpause(char* error, size_t maxlen) override;
    void AllPluginsLoaded() override;

    // hooks
    void OnLevelInit(char const* pMapName,
                     char const* pMapEntities,
                     char const* pOldLevel,
                     char const* pLandmarkName,
                     bool loadGame,
                     bool background) override;
    void OnLevelShutdown() override;

    const char* GetAuthor() override;
    const char* GetName() override;
    const char* GetDescription() override;
    const char* GetURL() override;
    const char* GetLicense() override;
    const char* GetVersion() override;
    const char* GetDate() override;
    const char* GetLogTag() override;

    void HookGameFrame(bool bSimulating, bool bFirstTick, bool bLastTick);
    static void HookStartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*);
    void AddTaskForNextFrame(std::function<void()>&& Task);

    static void HookRegisterLoopMode(const char* szLoopModeName, ILoopModeFactory* pLoopModeFactory, void** ppGlobalPointer);
    static IEngineService* HookFindService(const char* szServiceName);

  private:
    moodycamel::ConcurrentQueue<std::function<void()>> m_nextTasks;
};
} // namespace counterstrikesharp
