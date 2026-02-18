#pragma once
#include <string>
#include <functional>

using TokenResultCallback = std::function<void(bool success, const std::string& accessToken)>;

void startAuthServerAsync(int port,
                          TokenResultCallback callback);