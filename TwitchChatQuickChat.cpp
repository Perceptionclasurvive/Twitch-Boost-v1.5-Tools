#include "pch.h"
#include "TwitchChatQuickChat.h"
#include "Config.h"

BAKKESMOD_PLUGIN(TwitchChatQuickChat, "Twitch Chat Quick Chat", plugin_version,
    PLUGINTYPE_FREEPLAY | PLUGINTYPE_CUSTOM_TRAINING | PLUGINTYPE_SPECTATOR |
    PLUGINTYPE_BOTAI | PLUGINTYPE_REPLAY)

std::shared_ptr<CVarManagerWrapper> _globalCvarManager;

void TwitchChatQuickChat::onLoad()
{
    _globalCvarManager = cvarManager;

    // Initialize login module
    login_ = std::make_unique<Login>(gameWrapper);

    // Register CVars with persistence
    cvarManager->registerCvar("twitchChatQuickChat_chat_enabled", "0", "Enable Twitch Chat feature", true, true, 0, true, 1);
    cvarManager->registerCvar("twitchChatQuickChat_predictions_enabled", "0", "Enable Auto Predictions feature", true, true, 0, true, 1);
    cvarManager->registerCvar("twitchChatQuickChat_channel", "", "Twitch channel to join");

    // Load saved settings from cfg file
    cvarManager->loadCfg("twitchChatQuickChat.cfg");

    // Set up change listeners for features (will activate after login)
    cvarManager->getCvar("twitchChatQuickChat_chat_enabled").addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        if (login_ && login_->IsLoggedIn()) {
            if (cvar.getBoolValue()) {
                ConnectToTwitchChat();
            } else if (chat_) {
                chat_->Disconnect();
            }
        }
    });

    cvarManager->getCvar("twitchChatQuickChat_predictions_enabled").addOnValueChanged([this](std::string oldValue, CVarWrapper cvar) {
        if (login_ && login_->IsLoggedIn()) {
            if (cvar.getBoolValue()) {
                EnablePredictions();
            } else if (autoPredictions_) {
                autoPredictions_->Disable();
            }
        }
    });

    LOG("TwitchChatQuickChat: Plugin loaded");
}

void TwitchChatQuickChat::onUnload()
{
    // Save settings and disconnect
    cvarManager->backupCfg("twitchChatQuickChat.cfg");
    
    if (chat_) {
        chat_->Disconnect();
    }
    
    if (autoPredictions_) {
        autoPredictions_->Disable();
    }
}

void TwitchChatQuickChat::OnLoginComplete()
{
    //LOG("OnLoginComplete: username='{}', userId='{}'", login_->GetUsername(), login_->GetUserId());

    // Initialize features based on saved preferences
    CVarWrapper chatCvar = cvarManager->getCvar("twitchChatQuickChat_chat_enabled");
    if (chatCvar && chatCvar.getBoolValue()) {
        //LOG("OnLoginComplete: Chat is enabled, connecting...");
        ConnectToTwitchChat();
    }

    CVarWrapper predictionsCvar = cvarManager->getCvar("twitchChatQuickChat_predictions_enabled");
    if (predictionsCvar && predictionsCvar.getBoolValue()) {
        //LOG("OnLoginComplete: Predictions is enabled, enabling...");
        EnablePredictions();
    }
}

void TwitchChatQuickChat::ConnectToTwitchChat()
{
    if (!login_ || !login_->IsLoggedIn()) {
        return;
    }

    CVarWrapper channelCvar = cvarManager->getCvar("twitchChatQuickChat_channel");
    if (channelCvar) {
        twitchChannel_ = channelCvar.getStringValue();
    }

    if (twitchChannel_.empty()) {
        twitchChannel_ = login_->GetUsername();
    }

    // Remove # prefix if present
    if (!twitchChannel_.empty() && twitchChannel_[0] == '#') {
        twitchChannel_ = twitchChannel_.substr(1);
    }

    // Fetch the broadcaster's user ID, then connect
    login_->FetchBroadcasterId(twitchChannel_, [this](const std::string& fetchedId) {
        if (fetchedId.empty()) {
            return;
        }

        twitchChannelId_ = fetchedId;

        if (!chat_) {
            chat_ = std::make_unique<Chat>(gameWrapper);
        }

        chat_->Initialize(login_->GetAccessToken(), login_->GetUserId(), twitchChannelId_);
        chat_->Connect();
    });
}

void TwitchChatQuickChat::EnablePredictions()
{
    if (!login_ || !login_->IsLoggedIn() || login_->GetUserId().empty()) {
        return;
    }

    if (!autoPredictions_) {
        autoPredictions_ = std::make_unique<AutoPredictions>(gameWrapper, cvarManager);
    }

    //LOG("EnablePredictions: Calling Initialize with userId: {}", login_->GetUserId());
    autoPredictions_->Initialize(login_->GetAccessToken(), login_->GetUserId());
}

