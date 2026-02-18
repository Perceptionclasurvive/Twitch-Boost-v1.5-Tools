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

class TwitchWebSocket {
public:
    using MessageCallback = std::function<void(const std::string&)>;

    TwitchWebSocket();
    ~TwitchWebSocket();

    bool Connect(const std::string& accessToken, const std::string& nickname, const std::string& channel);
    void Disconnect();
    bool IsConnected() const;
    void SetMessageCallback(MessageCallback callback);
    bool SendMessage(const std::string& channel, const std::string& message);

private:
    void ReadLoop();
    bool PerformWebSocketHandshake();
    bool SendRaw(const std::string& data);
    std::string ReceiveFrame();
    void SendWebSocketFrame(const std::string& data);

    SOCKET socket_ = INVALID_SOCKET;
    SSL_CTX* sslCtx_ = nullptr;
    SSL* ssl_ = nullptr;
    std::atomic<bool> connected_{ false };
    std::thread readThread_;
    MessageCallback messageCallback_;
    std::string accessToken_;
    std::string nickname_;
    std::string channel_;
};