#include "pch.h"
#include "Chat.h"
#include "Config.h"
#include <thread>

Chat::Chat(std::shared_ptr<GameWrapper> gameWrapper)
    : gameWrapper_(gameWrapper)
{
}

void Chat::Initialize(const std::string& accessToken, const std::string& userId, const std::string& channelId)
{
    accessToken_ = accessToken;
    userId_ = userId;
    channelId_ = channelId;
}

void Chat::Connect()
{
    if (twitchEventSub_) {
        twitchEventSub_->Disconnect();
    }

    twitchEventSub_ = std::make_unique<TwitchEventSub>();
    twitchEventSub_->SetMessageCallback([this](const std::string& username, const std::string& message) {
        gameWrapper_->Execute([this, username, message](GameWrapper* gw) {
            OnTwitchMessage(username, message);
        });
    });

    std::thread([this]() {
        twitchEventSub_->Connect(accessToken_, Config::TWITCH_CLIENT_ID, userId_, channelId_);
    }).detach();
}

void Chat::Disconnect()
{
    if (twitchEventSub_) {
        twitchEventSub_->Disconnect();
    }
}

void Chat::OnTwitchMessage(const std::string& username, const std::string& message)
{
    std::string displayName = username;
    for (char& c : displayName) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    gameWrapper_->LogToChatbox(message, displayName);
}
