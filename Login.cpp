#include "pch.h"
#include "Login.h"
#include "Server.h"
#include "Config.h"
#include <thread>
#include <Windows.h>
#include <shellapi.h>
#include <httplib.h>

Login::Login(std::shared_ptr<GameWrapper> gameWrapper)
    : gameWrapper_(gameWrapper)
{
}

void Login::StartOAuthFlow(std::function<void(bool success)> onComplete) {
    isAuthenticating_ = true;

    // Build authorization URL for implicit flow
    std::string authUrl = "https://id.twitch.tv/oauth2/authorize"
        "?response_type=token"
        "&client_id=" + Config::TWITCH_CLIENT_ID +
        "&redirect_uri=" + Config::TWITCH_REDIRECT_URI +
        "&scope=" + "user:read:chat+channel:manage:predictions" +
        "&force_verify=true";

    // Start local server to receive the token
    startAuthServerAsync(3000, [this, onComplete](bool success, const std::string& token) {
        gameWrapper_->Execute([this, success, token, onComplete](GameWrapper* gw) {
            if (success && !token.empty()) {
                OnTokenReceived(token, onComplete);
            } else {
                isAuthenticating_ = false;
                if (onComplete) onComplete(false);
            }
        });
    });

    // Give server a moment to start, then open browser
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ShellExecuteA(nullptr, "open", authUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Login::OnTokenReceived(const std::string& accessToken, std::function<void(bool success)> onComplete) {
    accessToken_ = accessToken;

    // Fetch username and user ID from Twitch API
    std::thread([this, onComplete]() {
        httplib::SSLClient client("api.twitch.tv");
        client.set_connection_timeout(10);
        client.set_read_timeout(10);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + accessToken_},
            {"Client-Id", Config::TWITCH_CLIENT_ID}
        };

        auto result = client.Get("/helix/users", headers);

        std::string fetchedUsername = "unknown";
        std::string fetchedId;

        if (result && result->status == 200) {
            std::string body = result->body;

            // Parse login
            size_t loginPos = body.find("\"login\":\"");
            if (loginPos != std::string::npos) {
                size_t start = loginPos + 9;
                size_t end = body.find('"', start);
                if (end != std::string::npos) {
                    fetchedUsername = body.substr(start, end - start);
                }
            }

            // Parse user ID
            size_t idPos = body.find("\"id\":\"");
            if (idPos != std::string::npos) {
                size_t start = idPos + 6;
                size_t end = body.find('"', start);
                if (end != std::string::npos) {
                    fetchedId = body.substr(start, end - start);
                }
            }
        }

        gameWrapper_->Execute([this, fetchedUsername, fetchedId, onComplete](GameWrapper* gw) {
            username_ = fetchedUsername;
            userId_ = fetchedId;
            isLoggedIn_ = true;
            isAuthenticating_ = false;

            if (onComplete) onComplete(true);
        });
    }).detach();
}

void Login::FetchBroadcasterId(const std::string& channel, std::function<void(const std::string&)> callback) {
    std::thread([this, channel, callback]() {
        httplib::SSLClient client("api.twitch.tv");
        client.set_connection_timeout(10);
        client.set_read_timeout(10);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + accessToken_},
            {"Client-Id", Config::TWITCH_CLIENT_ID}
        };

        auto result = client.Get(("/helix/users?login=" + channel).c_str(), headers);

        std::string odId;
        if (result && result->status == 200) {
            std::string body = result->body;
            size_t idPos = body.find("\"id\":\"");
            if (idPos != std::string::npos) {
                size_t start = idPos + 6;
                size_t end = body.find('"', start);
                if (end != std::string::npos) {
                    odId = body.substr(start, end - start);
                }
            }
        }

        gameWrapper_->Execute([callback, odId](GameWrapper* gw) {
            callback(odId);
        });
    }).detach();
}