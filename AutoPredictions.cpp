#include "pch.h"
#include "AutoPredictions.h"
#include "Config.h"
#include <httplib.h>
#include <thread>

AutoPredictions::AutoPredictions(std::shared_ptr<GameWrapper> gameWrapper,
                                 std::shared_ptr<CVarManagerWrapper> cvarManager)
    : gameWrapper_(gameWrapper)
    , cvarManager_(cvarManager)
{
}

void AutoPredictions::Initialize(const std::string& accessToken, const std::string& broadcasterId)
{
    // Prevent double initialization
    if (initialized_) {
        //LOG("AutoPredictions: Already initialized, updating credentials");
        accessToken_ = accessToken;
        broadcasterId_ = broadcasterId;
        return;
    }

    accessToken_ = accessToken;
    broadcasterId_ = broadcasterId;

    // Hook for when the match countdown begins
    gameWrapper_->HookEvent("Function GameEvent_TA.Countdown.BeginState",
        [this](std::string eventName) {
            //LOG("AutoPredictions: Countdown.BeginState event fired");
            OnMatchStarted();
        });

    // Hook for when match ends and winner is determined
    gameWrapper_->HookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet",
        [this](std::string eventName) {
            //LOG("AutoPredictions: OnMatchWinnerSet event fired");
            OnMatchEnded();
        });

    // Hook for when player leaves the match (returns to main menu)
    gameWrapper_->HookEvent("Function TAGame.GFxData_MainMenu_TA.MainMenuAdded",
        [this](std::string eventName) {
            //LOG("AutoPredictions: MainMenuAdded event fired");
            OnPlayerLeftMatch();
        });

    // Hook for match destroyed/ended without winner
    gameWrapper_->HookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed",
        [this](std::string eventName) {
            //LOG("AutoPredictions: GameEvent destroyed");
            OnPlayerLeftMatch();
        });

    initialized_ = true;
    //LOG("AutoPredictions: Initialized and hooks registered");
}

void AutoPredictions::Disable()
{
    if (!initialized_) {
        return;
    }

    gameWrapper_->UnhookEvent("Function GameEvent_TA.Countdown.BeginState");
    gameWrapper_->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.OnMatchWinnerSet");
    gameWrapper_->UnhookEvent("Function TAGame.GFxData_MainMenu_TA.MainMenuAdded");
    gameWrapper_->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.Destroyed");

    // Cancel any active prediction
    if (predictionActive_ && !currentPredictionId_.empty()) {
        CancelPrediction();
    }

    initialized_ = false;
    //LOG("AutoPredictions: Disabled and hooks unregistered");
}

void AutoPredictions::OnMatchStarted()
{
    // Don't start prediction if one is already active
    if (predictionActive_) {
        //LOG("AutoPredictions: Prediction already active, skipping");
        return;
    }

    // Skip training modes and replays
    if (gameWrapper_->IsInFreeplay() || gameWrapper_->IsInCustomTraining() || gameWrapper_->IsInReplay()) {
        //LOG("AutoPredictions: In training/replay, skipping");
        return;
    }

    //LOG("AutoPredictions: Starting prediction creation");
    CreatePrediction();
}

void AutoPredictions::OnMatchEnded()
{
    // Only process if we have an active prediction
    if (!predictionActive_ || currentPredictionId_.empty()) {
        //LOG("AutoPredictions: No active prediction to resolve");
        return;
    }

    ServerWrapper server = gameWrapper_->GetCurrentGameState();
    if (!server) {
        //LOG("AutoPredictions: No server, canceling prediction");
        CancelPrediction();
        return;
    }

    // Get the winning team
    TeamWrapper winningTeam = server.GetMatchWinner();
    if (!winningTeam) {
        winningTeam = server.GetWinningTeam();
    }

    if (!winningTeam) {
        //LOG("AutoPredictions: No winning team, canceling prediction");
        CancelPrediction();
        return;
    }

    // Get the local player's team
    PlayerControllerWrapper localPlayer = gameWrapper_->GetPlayerController();
    if (!localPlayer) {
        //LOG("AutoPredictions: No local player, canceling prediction");
        CancelPrediction();
        return;
    }

    PriWrapper pri = localPlayer.GetPRI();
    if (!pri) {
        //LOG("AutoPredictions: No PRI, canceling prediction");
        CancelPrediction();
        return;
    }

    TeamInfoWrapper playerTeamInfo = pri.GetTeam();
    if (!playerTeamInfo) {
        //LOG("AutoPredictions: No team info, canceling prediction");
        CancelPrediction();
        return;
    }

    int playerTeamIndex = playerTeamInfo.GetTeamIndex();
    int winningTeamIndex = winningTeam.GetTeamIndex();

    bool playerWon = (winningTeamIndex == playerTeamIndex);
    //LOG("AutoPredictions: Player {} (team {} vs winner {})", playerWon ? "WON" : "LOST", playerTeamIndex, winningTeamIndex);

    std::string winningOutcomeId = playerWon ? outcomeWinId_ : outcomeLoseId_;
    ResolvePrediction(winningOutcomeId);
}

void AutoPredictions::OnPlayerLeftMatch()
{
    // If player leaves without winner being set, determine outcome from game state
    if (!predictionActive_ || currentPredictionId_.empty() || outcomeLoseId_.empty()) {
        return;
    }

    //LOG("AutoPredictions: Player left match, checking game state");

    // Try to determine winner from current score
    std::string outcomeId = DetermineOutcomeFromGameState();
    
    if (outcomeId.empty()) {
        // Couldn't determine, default to loss (player quit/forfeited)
        //LOG("AutoPredictions: Could not determine winner, defaulting to L");
        outcomeId = outcomeLoseId_;
    }

    ResolvePrediction(outcomeId);
}

std::string AutoPredictions::DetermineOutcomeFromGameState()
{
    ServerWrapper server = gameWrapper_->GetCurrentGameState();
    if (!server) {
        //LOG("AutoPredictions: No server available");
        return "";
    }

    // Check if there's already a match winner set
    TeamWrapper matchWinner = server.GetMatchWinner();
    if (matchWinner) {
        return GetOutcomeForWinner(matchWinner);
    }

    // Check if there's a game winner (current game in series)
    TeamWrapper gameWinner = server.GetGameWinner();
    if (gameWinner) {
        return GetOutcomeForWinner(gameWinner);
    }

    // In overtime - check if one team is ahead (they've scored the OT goal)
    if (server.GetbOverTime()) {
        ArrayWrapper<TeamWrapper> teams = server.GetTeams();
        if (teams.Count() >= 2) {
            int score0 = teams.Get(0).GetScore();
            int score1 = teams.Get(1).GetScore();
            
            //LOG("AutoPredictions: Overtime scores - Team0: {}, Team1: {}", score0, score1);
            
            if (score0 != score1) {
                // Someone scored in OT - determine winner
                int winningTeamIndex = (score0 > score1) ? 0 : 1;
                int playerTeamIndex = GetPlayerTeamIndex();
                
                if (playerTeamIndex >= 0) {
                    bool playerWon = (winningTeamIndex == playerTeamIndex);
                    //LOG("AutoPredictions: OT winner determined - Player {} (team {} vs winner {})", playerWon ? "WON" : "LOST", playerTeamIndex, winningTeamIndex);
                    return playerWon ? outcomeWinId_ : outcomeLoseId_;
                }
            }
        }
    }

    // Not in overtime or scores are tied - player is leaving early, count as loss
    return "";
}

std::string AutoPredictions::GetOutcomeForWinner(TeamWrapper winningTeam)
{
    int playerTeamIndex = GetPlayerTeamIndex();
    if (playerTeamIndex < 0) {
        return "";
    }

    int winningTeamIndex = winningTeam.GetTeamIndex();
    bool playerWon = (winningTeamIndex == playerTeamIndex);
    
    //LOG("AutoPredictions: Winner determined - Player {} (team {} vs winner {})", playerWon ? "WON" : "LOST", playerTeamIndex, winningTeamIndex);
    
    return playerWon ? outcomeWinId_ : outcomeLoseId_;
}

int AutoPredictions::GetPlayerTeamIndex()
{
    PlayerControllerWrapper localPlayer = gameWrapper_->GetPlayerController();
    if (!localPlayer) {
        return -1;
    }

    PriWrapper pri = localPlayer.GetPRI();
    if (!pri) {
        return -1;
    }

    TeamInfoWrapper playerTeamInfo = pri.GetTeam();
    if (!playerTeamInfo) {
        return -1;
    }

    return playerTeamInfo.GetTeamIndex();
}

bool AutoPredictions::HasActivePrediction()
{
    std::string status = GetPredictionStatus();
    return status == "ACTIVE" || status == "LOCKED";
}

std::string AutoPredictions::GetPredictionStatus()
{
    httplib::SSLClient client("api.twitch.tv");
    client.set_connection_timeout(5);
    client.set_read_timeout(5);

    httplib::Headers headers = {
        {"Authorization", "Bearer " + accessToken_},
        {"Client-Id", Config::TWITCH_CLIENT_ID}
    };

    auto result = client.Get(("/helix/predictions?broadcaster_id=" + broadcasterId_).c_str(), headers);

    if (!result || result->status != 200) {
        //LOG("AutoPredictions: Failed to get prediction status");
        return "";
    }

    const std::string& body = result->body;
    
    // Find status in response
    size_t statusPos = body.find("\"status\":\"");
    if (statusPos != std::string::npos) {
        size_t start = statusPos + 10;
        size_t end = body.find('"', start);
        if (end != std::string::npos) {
            std::string status = body.substr(start, end - start);
            //LOG("AutoPredictions: Current prediction status: {}", status);
            return status;
        }
    }

    return "";
}

void AutoPredictions::CreatePrediction()
{
    //LOG("AutoPredictions: Creating prediction on Twitch");
    //LOG("AutoPredictions: Broadcaster ID: {}", broadcasterId_);

    std::thread([this]() {
        // Check if there's already an active prediction on Twitch
        if (HasActivePrediction()) {
            //LOG("AutoPredictions: Active prediction already exists on Twitch, skipping");
            return;
        }

        httplib::SSLClient client("api.twitch.tv");
        client.set_connection_timeout(10);
        client.set_read_timeout(10);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + accessToken_},
            {"Client-Id", Config::TWITCH_CLIENT_ID},
            {"Content-Type", "application/json"}
        };

        // Build JSON body for prediction (compact JSON, no extra whitespace)
        std::string body = R"({"broadcaster_id":")" + broadcasterId_ +
            R"(","title":"W or L?","outcomes":[{"title":"W"},{"title":"L"}],"prediction_window":120})";

        //LOG("AutoPredictions: Request body: {}", body);

        auto result = client.Post("/helix/predictions", headers, body, "application/json");

        if (!result) {
            //LOG("AutoPredictions: No response from Twitch API (connection failed)");
            return;
        }

        //LOG("AutoPredictions: Response status: {}", result->status);
        //LOG("AutoPredictions: Response body: {}", result->body);

        if (result->status == 200) {
            std::string responseBody = result->body;

            size_t idPos = responseBody.find("\"id\":\"");
            if (idPos != std::string::npos) {
                size_t start = idPos + 6;
                size_t end = responseBody.find('"', start);
                if (end != std::string::npos) {
                    std::string predictionId = responseBody.substr(start, end - start);

                    std::string outcomeWinId, outcomeLoseId;

                    size_t outcomesPos = responseBody.find("\"outcomes\":");
                    if (outcomesPos != std::string::npos) {
                        size_t firstOutcomeIdPos = responseBody.find("\"id\":\"", outcomesPos);
                        if (firstOutcomeIdPos != std::string::npos) {
                            size_t s1 = firstOutcomeIdPos + 6;
                            size_t e1 = responseBody.find('"', s1);
                            if (e1 != std::string::npos) {
                                outcomeWinId = responseBody.substr(s1, e1 - s1);
                            }

                            size_t secondOutcomeIdPos = responseBody.find("\"id\":\"", e1);
                            if (secondOutcomeIdPos != std::string::npos) {
                                size_t s2 = secondOutcomeIdPos + 6;
                                size_t e2 = responseBody.find('"', s2);
                                if (e2 != std::string::npos) {
                                    outcomeLoseId = responseBody.substr(s2, e2 - s2);
                                }
                            }
                        }
                    }

                    //LOG("AutoPredictions: Prediction ID: {}", predictionId);
                    //LOG("AutoPredictions: Win outcome ID: {}", outcomeWinId);
                    //LOG("AutoPredictions: Lose outcome ID: {}", outcomeLoseId);

                    gameWrapper_->Execute([this, predictionId, outcomeWinId, outcomeLoseId](GameWrapper* gw) {
                        currentPredictionId_ = predictionId;
                        outcomeWinId_ = outcomeWinId;
                        outcomeLoseId_ = outcomeLoseId;
                        predictionActive_ = true;
                        //LOG("AutoPredictions: Prediction state updated, active = true");
                    });
                }
            }
        } else {
            //LOG("AutoPredictions: API error - Status {}: {}", result->status, result->body);
        }
    }).detach();
}

void AutoPredictions::ResolvePrediction(const std::string& winningOutcomeId)
{
    // Capture all values before clearing
    std::string predictionId = currentPredictionId_;
    std::string outcomeId = winningOutcomeId;

    // Guard against empty outcome ID
    if (predictionId.empty() || outcomeId.empty()) {
        //LOG("AutoPredictions: Cannot resolve - missing prediction ID or outcome ID");
        predictionActive_ = false;
        currentPredictionId_.clear();
        outcomeWinId_.clear();
        outcomeLoseId_.clear();
        return;
    }

    predictionActive_ = false;
    currentPredictionId_.clear();
    outcomeWinId_.clear();
    outcomeLoseId_.clear();

    //LOG("AutoPredictions: Resolving prediction {} with outcome {}", predictionId, outcomeId);

    std::thread([this, predictionId, outcomeId]() {
        // Check if prediction is still in ACTIVE state (voting window open)
        std::string status = GetPredictionStatus();
        if (status == "ACTIVE") {
            //LOG("AutoPredictions: Prediction still ACTIVE (voting open), canceling instead of resolving");
            
            httplib::SSLClient client("api.twitch.tv");
            client.set_connection_timeout(10);
            client.set_read_timeout(10);

            httplib::Headers headers = {
                {"Authorization", "Bearer " + accessToken_},
                {"Client-Id", Config::TWITCH_CLIENT_ID},
                {"Content-Type", "application/json"}
            };

            std::string body = R"({"broadcaster_id":")" + broadcasterId_ +
                R"(","id":")" + predictionId + R"(","status":"CANCELED"})";

            auto result = client.Patch("/helix/predictions", headers, body, "application/json");

            if (result) {
                //LOG("AutoPredictions: Cancel response - Status {}: {}", result->status, result->body);
            }
            return;
        }

        // Prediction is LOCKED, proceed with resolve
        httplib::SSLClient client("api.twitch.tv");
        client.set_connection_timeout(10);
        client.set_read_timeout(10);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + accessToken_},
            {"Client-Id", Config::TWITCH_CLIENT_ID},
            {"Content-Type", "application/json"}
        };

        std::string body = R"({"broadcaster_id":")" + broadcasterId_ +
            R"(","id":")" + predictionId +
            R"(","status":"RESOLVED","winning_outcome_id":")" + outcomeId + R"("})";

        auto result = client.Patch("/helix/predictions", headers, body, "application/json");

        if (result) {
            //LOG("AutoPredictions: Resolve response - Status {}: {}", result->status, result->body);
        } else {
            //LOG("AutoPredictions: Resolve failed - no response");
        }
    }).detach();
}

void AutoPredictions::CancelPrediction()
{
    std::string predictionId = currentPredictionId_;

    predictionActive_ = false;
    currentPredictionId_.clear();
    outcomeWinId_.clear();
    outcomeLoseId_.clear();

    //LOG("AutoPredictions: Canceling prediction {}", predictionId);

    std::thread([this, predictionId]() {
        httplib::SSLClient client("api.twitch.tv");
        client.set_connection_timeout(10);
        client.set_read_timeout(10);

        httplib::Headers headers = {
            {"Authorization", "Bearer " + accessToken_},
            {"Client-Id", Config::TWITCH_CLIENT_ID},
            {"Content-Type", "application/json"}
        };

        std::string body = R"({"broadcaster_id":")" + broadcasterId_ +
            R"(","id":")" + predictionId + R"(","status":"CANCELED"})";

        auto result = client.Patch("/helix/predictions", headers, body, "application/json");

        if (result) {
            //LOG("AutoPredictions: Cancel response - Status {}: {}", result->status, result->body);
        }
    }).detach();
}