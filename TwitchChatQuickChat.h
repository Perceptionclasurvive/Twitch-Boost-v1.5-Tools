#pragma once

#include "GuiBase.h"
#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"
#include "Login.h"
#include "Chat.h"
#include "AutoPredictions.h"
#include "version.h"

constexpr auto plugin_version = stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." stringify(VERSION_PATCH) "." stringify(VERSION_BUILD);

class TwitchChatQuickChat: public BakkesMod::Plugin::BakkesModPlugin
    ,public SettingsWindowBase
{
    // Feature modules
    std::unique_ptr<Login> login_;
    std::unique_ptr<Chat> chat_;
    std::unique_ptr<AutoPredictions> autoPredictions_;

    // Channel state
    std::string twitchChannel_;
    std::string twitchChannelId_;

    void onLoad() override;
    void onUnload() override;

    // Helper methods called by CVars and settings
    void ConnectToTwitchChat();
    void EnablePredictions();
    void OnLoginComplete();

public:
    void RenderSettings() override;
};