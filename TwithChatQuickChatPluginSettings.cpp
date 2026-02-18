# include "pch.h"
# include "TwitchChatQuickChat.h"

void TwitchChatQuickChat::RenderSettings() {
    // Login section
    if (!login_ || !login_->IsLoggedIn()) {
        if (login_ && login_->IsAuthenticating()) {
            ImGui::TextUnformatted("Authenticating... Please complete login in your browser.");
        } else {
            if (ImGui::Button("Login with Twitch")) {
                login_->StartOAuthFlow([this](bool success) {
                    if (success) {
                        OnLoginComplete();
                    }
                });
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to authenticate with your Twitch account");
            }
        }
        return;
    }

    // Logged in - show username
    ImGui::Text("Logged in as: %s", login_->GetUsername().c_str());
    ImGui::Spacing();

    // Tabs for different features
    if (ImGui::BeginTabBar("FeatureTabs")) {
        // Chat Tab
        if (ImGui::BeginTabItem("Chat")) {
            ImGui::TextUnformatted("Display Twitch chat messages in-game");
            ImGui::Spacing();

            CVarWrapper chatCvar = cvarManager->getCvar("twitchChatQuickChat_chat_enabled");
            if (chatCvar) {
                bool chatEnabled = chatCvar.getBoolValue();
                if (ImGui::Checkbox("Enable Chat", &chatEnabled)) {
                    chatCvar.setValue(chatEnabled);
                    // Save settings
                    cvarManager->backupCfg("twitchChatQuickChat.cfg");
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Show Twitch chat messages in the game chatbox");
                }
            }

            ImGui::EndTabItem();
        }

        // Predictions Tab
        if (ImGui::BeginTabItem("Predictions")) {
            ImGui::TextUnformatted("Automatically create W/L predictions for matches");
            ImGui::Spacing();

            CVarWrapper predictionsCvar = cvarManager->getCvar("twitchChatQuickChat_predictions_enabled");
            if (predictionsCvar) {
                bool predictionsEnabled = predictionsCvar.getBoolValue();
                if (ImGui::Checkbox("Enable Auto Predictions", &predictionsEnabled)) {
                    predictionsCvar.setValue(predictionsEnabled);
                    // Save settings
                    cvarManager->backupCfg("twitchChatQuickChat.cfg");
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Automatically start a 'W or L?' prediction when a match begins");
                }
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
}

