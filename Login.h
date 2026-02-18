#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <string>
#include <memory>
#include <functional>

class Login
{
public:
    Login(std::shared_ptr<GameWrapper> gameWrapper);

    void StartOAuthFlow(std::function<void(bool success)> onComplete);
    void FetchBroadcasterId(const std::string& channel, std::function<void(const std::string&)> callback);

    // Getters for auth state
    bool IsLoggedIn() const { return isLoggedIn_; }
    bool IsAuthenticating() const { return isAuthenticating_; }
    const std::string& GetAccessToken() const { return accessToken_; }
    const std::string& GetUsername() const { return username_; }
    const std::string& GetUserId() const { return userId_; }

private:
    void OnTokenReceived(const std::string& accessToken, std::function<void(bool success)> onComplete);

    std::shared_ptr<GameWrapper> gameWrapper_;

    std::string accessToken_;
    std::string username_;
    std::string userId_;
    bool isLoggedIn_ = false;
    bool isAuthenticating_ = false;
};
