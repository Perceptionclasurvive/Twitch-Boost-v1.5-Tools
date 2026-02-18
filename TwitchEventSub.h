#pragma once
#include <string>
#include <functional>
#include <thread>
#include <atomic>

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma comment(lib, "ws2_32.lib")

class TwitchEventSub {
public:
    using MessageCallback = std::function<void(const std::string& username, const std::string& message)>;

    TwitchEventSub();
    ~TwitchEventSub();

    bool Connect(const std::string& accessToken, const std::string& clientId, 
                 const std::string& userId, const std::string& broadcasterId);
    void Disconnect();
    bool IsConnected() const;
    void SetMessageCallback(MessageCallback callback);

private:
    void ReadLoop();
    bool PerformWebSocketHandshake();
    std::string ReceiveFrame();
    void SendWebSocketFrame(const std::string& data);
    bool SubscribeToChatMessages(const std::string& sessionId);
    void HandleMessage(const std::string& payload);
    std::string ParseJsonString(const std::string& json, const std::string& key);

    SOCKET socket_ = INVALID_SOCKET;
    SSL_CTX* sslCtx_ = nullptr;
    SSL* ssl_ = nullptr;
    std::atomic<bool> connected_{ false };
    std::thread readThread_;
    MessageCallback messageCallback_;
    
    std::string accessToken_;
    std::string clientId_;
    std::string userId_;
    std::string broadcasterId_;
};