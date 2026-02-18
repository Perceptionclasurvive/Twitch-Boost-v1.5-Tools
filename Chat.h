#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "TwitchEventSub.h"
#include <string>
#include <string>
#include <memory>

class Chat
{
public:
    Chat(std::shared_ptr<GameWrapper> gameWrapper);

    void Initialize(const std::string& accessToken, const std::string& userId, const std::string& channelId);
    void Connect();
    void Disconnect();

private:
    void OnTwitchMessage(const std::string& username, const std::string& message);

    std::shared_ptr<GameWrapper> gameWrapper_;

    std::string accessToken_;
    std::string userId_;
    std::string channelId_;

    std::unique_ptr<TwitchEventSub> twitchEventSub_;
};
