#include "pch.h"
#include "Server.h"
#include <thread>
#include <mutex>
#include <httplib.h>

void startAuthServerAsync(int port, TokenResultCallback callback) {
    std::thread([port, callback = std::move(callback)]() {
        httplib::Server svr;
        bool callbackFired = false;
        std::mutex mtx;

        // Serve a page with JavaScript to extract the token from the fragment
        svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
            res.set_content(R"html(
<!DOCTYPE html>
<html>
<head><title>Twitch Authorization</title></head>
<body>
    <h1>Processing Authorization...</h1>
    <p id="status">Please wait...</p>
    <script>
        // The token is in the URL fragment (after #)
        var fragment = window.location.hash.substring(1);
        var params = new URLSearchParams(fragment);
        var accessToken = params.get('access_token');
        var error = params.get('error');
        
        if (accessToken) {
            document.getElementById('status').textContent = 'Token received, completing...';
            // Send token to our server
            fetch('/token?access_token=' + encodeURIComponent(accessToken))
                .then(function(response) { return response.text(); })
                .then(function(result) {
                    document.getElementById('status').textContent = 'Success! You can close this window.';
                    setTimeout(function() { window.close(); }, 2000);
                })
                .catch(function(err) {
                    document.getElementById('status').textContent = 'Error: ' + err;
                });
        } else if (error) {
            document.getElementById('status').textContent = 'Authorization failed: ' + error;
            fetch('/token?error=' + encodeURIComponent(error));
        } else {
            document.getElementById('status').textContent = 'No token received. Please try again.';
            fetch('/token?error=no_token');
        }
    </script>
</body>
</html>
)html", "text/html");
        });

        // Endpoint to receive the token from JavaScript
        svr.Get("/token", [&](const httplib::Request& req, httplib::Response& res) {
            std::lock_guard<std::mutex> lock(mtx);
            
            if (callbackFired) {
                res.set_content("Already processed", "text/plain");
                return;
            }
            
            auto tokenIt = req.params.find("access_token");
            if (tokenIt != req.params.end() && !tokenIt->second.empty()) {
                std::string accessToken = tokenIt->second;
                callbackFired = true;
                
                res.set_content("OK", "text/plain");
                svr.stop();
                callback(true, accessToken);
            } else {
                auto errIt = req.params.find("error");
                std::string errorMsg = errIt != req.params.end() ? errIt->second : "Unknown error";
                
                callbackFired = true;
                res.set_content("Error: " + errorMsg, "text/plain");
                svr.stop();
                callback(false, "");
            }
        });

        svr.listen("127.0.0.1", port);
    }).detach();
}