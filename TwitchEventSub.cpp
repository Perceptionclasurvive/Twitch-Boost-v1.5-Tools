#include "pch.h"
#include "TwitchEventSub.h"
#include "logging.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <httplib.h>

TwitchEventSub::TwitchEventSub() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
}

TwitchEventSub::~TwitchEventSub() {
    Disconnect();
    WSACleanup();
}

bool TwitchEventSub::Connect(const std::string& accessToken, const std::string& clientId,
                              const std::string& userId, const std::string& broadcasterId) {
    accessToken_ = accessToken;
    clientId_ = clientId;
    userId_ = userId;
    broadcasterId_ = broadcasterId;

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
    
    if (getaddrinfo("eventsub.wss.twitch.tv", "443", &hints, &result) != 0) {
        //LOG("Failed to resolve EventSub hostname");
        closesocket(socket_);
        return false;
    }

    // Connect
    if (connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR) {
        //LOG("Failed to connect to EventSub");
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
    SSL_set_tlsext_host_name(ssl_, "eventsub.wss.twitch.tv");

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

    connected_ = true;

    // Start read loop - subscription happens after receiving session_welcome
    readThread_ = std::thread(&TwitchEventSub::ReadLoop, this);

    //LOG("Connected to Twitch EventSub WebSocket");
    return true;
}

void TwitchEventSub::Disconnect() {
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

bool TwitchEventSub::IsConnected() const {
    return connected_;
}

void TwitchEventSub::SetMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

bool TwitchEventSub::PerformWebSocketHandshake() {
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
    request << "GET /ws HTTP/1.1\r\n"
            << "Host: eventsub.wss.twitch.tv\r\n"
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

void TwitchEventSub::SendWebSocketFrame(const std::string& data) {
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

std::string TwitchEventSub::ReceiveFrame() {
    unsigned char header[2];
    if (SSL_read(ssl_, header, 2) != 2) {
        return "";
    }

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

std::string TwitchEventSub::ParseJsonString(const std::string& json, const std::string& key) {
    std::string searchKey = "\"" + key + "\":\"";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) {
        // Try without quotes for nested objects
        searchKey = "\"" + key + "\":";
        pos = json.find(searchKey);
        if (pos == std::string::npos) return "";
        
        size_t start = pos + searchKey.length();
        if (start < json.length() && json[start] == '"') {
            start++;
            size_t end = json.find('"', start);
            if (end != std::string::npos) {
                return json.substr(start, end - start);
            }
        }
        return "";
    }
    
    size_t start = pos + searchKey.length();
    size_t end = json.find('"', start);
    if (end == std::string::npos) return "";
    
    return json.substr(start, end - start);
}

bool TwitchEventSub::SubscribeToChatMessages(const std::string& sessionId) {
    //LOG("Subscribing to chat messages with session: {}", sessionId);
    
    // Use httplib to make the subscription request
    httplib::SSLClient client("api.twitch.tv");
    client.set_connection_timeout(10);
    client.set_read_timeout(10);
    
    httplib::Headers headers = {
        {"Authorization", "Bearer " + accessToken_},
        {"Client-Id", clientId_},
        {"Content-Type", "application/json"}
    };
    
    // Build subscription request JSON
    std::ostringstream json;
    json << "{"
         << "\"type\":\"channel.chat.message\","
         << "\"version\":\"1\","
         << "\"condition\":{"
         << "\"broadcaster_user_id\":\"" << broadcasterId_ << "\","
         << "\"user_id\":\"" << userId_ << "\""
         << "},"
         << "\"transport\":{"
         << "\"method\":\"websocket\","
         << "\"session_id\":\"" << sessionId << "\""
         << "}"
         << "}";
    
    //LOG("Subscription request: {}", json.str());
    
    auto result = client.Post("/helix/eventsub/subscriptions", headers, json.str(), "application/json");
    
    if (!result) {
        //LOG("Subscription request failed - no response");
        return false;
    }
    
    //LOG("Subscription response: {} - {}", result->status, result->body);
    
    if (result->status == 202) {
        //LOG("Successfully subscribed to chat messages");
        return true;
    }
    
    //LOG("Subscription failed with status {}", result->status);
    return false;
}

void TwitchEventSub::HandleMessage(const std::string& payload) {
    //LOG("EventSub message: {}", payload);
    
    // Handle session_welcome
    if (payload.find("\"session_welcome\"") != std::string::npos || 
        payload.find("\"message_type\":\"session_welcome\"") != std::string::npos) {
        std::string sessionId = ParseJsonString(payload, "id");
        if (!sessionId.empty()) {
            //LOG("Received session_welcome with id: {}", sessionId);
            // Subscribe in a separate thread to not block the read loop
            std::thread([this, sessionId]() {
                SubscribeToChatMessages(sessionId);
            }).detach();
        }
        return;
    }
    
    // Handle session_keepalive - just ignore, connection is alive
    if (payload.find("\"session_keepalive\"") != std::string::npos) {
        return;
    }
    
    // Handle notification (chat message)
    if (payload.find("\"notification\"") != std::string::npos && 
        payload.find("\"channel.chat.message\"") != std::string::npos) {
        
        // Parse the chat message from the event
        std::string chatterName = ParseJsonString(payload, "chatter_user_name");
        std::string messageText = ParseJsonString(payload, "message_text");
        
        // If message_text not found, try just "text" in message object
        if (messageText.empty()) {
            size_t messagePos = payload.find("\"message\":{");
            if (messagePos != std::string::npos) {
                std::string messageSection = payload.substr(messagePos);
                messageText = ParseJsonString(messageSection, "text");
            }
        }
        
        if (!chatterName.empty() && !messageText.empty() && messageCallback_) {
            messageCallback_(chatterName, messageText);
        }
        return;
    }
}

void TwitchEventSub::ReadLoop() {
    while (connected_) {
        std::string frame = ReceiveFrame();
        if (frame.empty()) {
            continue;
        }
        
        HandleMessage(frame);
    }
}