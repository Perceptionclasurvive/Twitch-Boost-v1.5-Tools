#include "pch.h"
#include "TwitchWebSocket.h"
#include "logging.h"
#include <random>
#include <sstream>
#include <iomanip>

TwitchWebSocket::TwitchWebSocket() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

TwitchWebSocket::~TwitchWebSocket() {
    Disconnect();
    WSACleanup();
}

bool TwitchWebSocket::Connect(const std::string& accessToken, const std::string& nickname, const std::string& channel) {
    accessToken_ = accessToken;
    nickname_ = nickname;
    channel_ = channel;

    // Create socket
    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        //LOG("Failed to create socket");
        return false;
    }

    // Resolve hostname
    struct addrinfo hints = {}, *result = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo("irc-ws.chat.twitch.tv", "443", &hints, &result) != 0) {
        //LOG("Failed to resolve Twitch IRC hostname");
        closesocket(socket_);
        return false;
    }

    // Connect
    if (connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
        //LOG("Failed to connect to Twitch IRC");
        freeaddrinfo(result);
        closesocket(socket_);
        return false;
    }
    freeaddrinfo(result);

    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    
    sslCtx_ = SSL_CTX_new(TLS_client_method());
    if (!sslCtx_) {
        //LOG("Failed to create SSL context");
        closesocket(socket_);
        return false;
    }

    ssl_ = SSL_new(sslCtx_);
    SSL_set_fd(ssl_, static_cast<int>(socket_));
    SSL_set_tlsext_host_name(ssl_, "irc-ws.chat.twitch.tv");

    if (SSL_connect(ssl_) != 1) {
        //LOG("SSL handshake failed");
        SSL_free(ssl_);
        SSL_CTX_free(sslCtx_);
        closesocket(socket_);
        return false;
    }

    // Perform WebSocket handshake
    if (!PerformWebSocketHandshake()) {
        //LOG("WebSocket handshake failed");
        Disconnect();
        return false;
    }

    // Send IRC authentication
    SendWebSocketFrame("CAP REQ :twitch.tv/tags twitch.tv/commands");
    SendWebSocketFrame("PASS oauth:" + accessToken_);
    SendWebSocketFrame("NICK " + nickname_);
    SendWebSocketFrame("JOIN #" + channel_);

    connected_ = true;

    // Start read loop
    readThread_ = std::thread(&TwitchWebSocket::ReadLoop, this);

    //LOG("Connected to Twitch IRC for channel: #{}", channel_);
    return true;
}

void TwitchWebSocket::Disconnect() {
    connected_ = false;
    
    if (readThread_.joinable()) {
        readThread_.join();
    }

    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }

    if (sslCtx_) {
        SSL_CTX_free(sslCtx_);
        sslCtx_ = nullptr;
    }

    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}

bool TwitchWebSocket::IsConnected() const {
    return connected_;
}

void TwitchWebSocket::SetMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

bool TwitchWebSocket::PerformWebSocketHandshake() {
    // Generate random WebSocket key
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    
    unsigned char keyBytes[16];
    for (int i = 0; i < 16; ++i) {
        keyBytes[i] = static_cast<unsigned char>(dist(gen));
    }
    
    // Base64 encode the key
    static const char* b64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string wsKey;
    int val = 0, valb = -6;
    for (int i = 0; i < 16; ++i) {
        val = (val << 8) + keyBytes[i];
        valb += 8;
        while (valb >= 0) {
            wsKey.push_back(b64[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) wsKey.push_back(b64[((val << 8) >> (valb + 8)) & 0x3F]);
    while (wsKey.size() % 4) wsKey.push_back('=');

    // Send HTTP upgrade request
    std::ostringstream request;
    request << "GET / HTTP/1.1\r\n"
            << "Host: irc-ws.chat.twitch.tv\r\n"
            << "Upgrade: websocket\r\n"
            << "Connection: Upgrade\r\n"
            << "Sec-WebSocket-Key: " << wsKey << "\r\n"
            << "Sec-WebSocket-Version: 13\r\n"
            << "\r\n";

    std::string reqStr = request.str();
    if (SSL_write(ssl_, reqStr.c_str(), static_cast<int>(reqStr.size())) <= 0) {
        return false;
    }

    // Read response
    char buffer[1024];
    int bytesRead = SSL_read(ssl_, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        return false;
    }
    buffer[bytesRead] = '\0';

    // Check for 101 Switching Protocols
    std::string response(buffer);
    return response.find("101") != std::string::npos;
}

void TwitchWebSocket::SendWebSocketFrame(const std::string& data) {
    std::vector<unsigned char> frame;
    
    // Text frame opcode with FIN bit
    frame.push_back(0x81);
    
    // Mask bit set + payload length
    size_t len = data.size();
    if (len <= 125) {
        frame.push_back(static_cast<unsigned char>(0x80 | len));
    } else if (len <= 65535) {
        frame.push_back(0xFE);
        frame.push_back(static_cast<unsigned char>((len >> 8) & 0xFF));
        frame.push_back(static_cast<unsigned char>(len & 0xFF));
    } else {
        frame.push_back(0xFF);
        for (int i = 7; i >= 0; --i) {
            frame.push_back(static_cast<unsigned char>((len >> (8 * i)) & 0xFF));
        }
    }

    // Masking key
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 255);
    unsigned char mask[4];
    for (int i = 0; i < 4; ++i) {
        mask[i] = static_cast<unsigned char>(dist(gen));
        frame.push_back(mask[i]);
    }

    // Masked payload
    for (size_t i = 0; i < data.size(); ++i) {
        frame.push_back(data[i] ^ mask[i % 4]);
    }

    SSL_write(ssl_, frame.data(), static_cast<int>(frame.size()));
}

std::string TwitchWebSocket::ReceiveFrame() {
    unsigned char header[2];
    if (SSL_read(ssl_, header, 2) != 2) {
        return "";
    }

    bool fin = (header[0] & 0x80) != 0;
    int opcode = header[0] & 0x0F;
    bool masked = (header[1] & 0x80) != 0;
    uint64_t payloadLen = header[1] & 0x7F;

    if (payloadLen == 126) {
        unsigned char extLen[2];
        SSL_read(ssl_, extLen, 2);
        payloadLen = (extLen[0] << 8) | extLen[1];
    } else if (payloadLen == 127) {
        unsigned char extLen[8];
        SSL_read(ssl_, extLen, 8);
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | extLen[i];
        }
    }

    unsigned char mask[4] = {0};
    if (masked) {
        SSL_read(ssl_, mask, 4);
    }

    std::string payload;
    payload.resize(static_cast<size_t>(payloadLen));
    if (payloadLen > 0) {
        SSL_read(ssl_, &payload[0], static_cast<int>(payloadLen));
        if (masked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] ^= mask[i % 4];
            }
        }
    }

    // Handle ping - respond with pong
    if (opcode == 0x09) {
        std::vector<unsigned char> pong;
        pong.push_back(0x8A); // Pong frame
        pong.push_back(0x80); // Masked, zero length
        unsigned char pongMask[4] = {0, 0, 0, 0};
        pong.insert(pong.end(), pongMask, pongMask + 4);
        SSL_write(ssl_, pong.data(), static_cast<int>(pong.size()));
        return "";
    }

    // Close frame
    if (opcode == 0x08) {
        connected_ = false;
        return "";
    }

    return payload;
}

void TwitchWebSocket::ReadLoop() {
    while (connected_) {
        std::string frame = ReceiveFrame();
        if (frame.empty()) {
            continue;
        }

        // Handle IRC PING
        if (frame.find("PING") == 0) {
            std::string pong = "PONG" + frame.substr(4);
            SendWebSocketFrame(pong);
            continue;
        }

        // Pass the full frame to the callback for PRIVMSG messages
        if (frame.find("PRIVMSG") != std::string::npos && messageCallback_) {
            // Remove trailing \r\n
            while (!frame.empty() && (frame.back() == '\r' || frame.back() == '\n')) {
                frame.pop_back();
            }
            messageCallback_(frame);  // Pass full frame, not just the message text
        }
    }
}

bool TwitchWebSocket::SendMessage(const std::string& channel, const std::string& message) {
    if (!connected_) return false;
    SendWebSocketFrame("PRIVMSG #" + channel + " :" + message);
    return true;
}