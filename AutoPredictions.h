#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include <string>
#include <memory>

class AutoPredictions
{
public:
    AutoPredictions(std::shared_ptr<GameWrapper> gameWrapper, 
                    std::shared_ptr<CVarManagerWrapper> cvarManager);
    
    void Initialize(const std::string& accessToken, const std::string& broadcasterId);
    void Disable();
    
private:
    void OnMatchStarted();
    void OnMatchEnded();
    void OnPlayerLeftMatch();
    
    bool HasActivePrediction();
    std::string GetPredictionStatus();
    std::string DetermineOutcomeFromGameState();
    std::string GetOutcomeForWinner(TeamWrapper winningTeam);
    int GetPlayerTeamIndex();
    
    void CreatePrediction();
    void ResolvePrediction(const std::string& winningOutcomeId);
    void CancelPrediction();
    
    std::shared_ptr<GameWrapper> gameWrapper_;
    std::shared_ptr<CVarManagerWrapper> cvarManager_;
    
    std::string accessToken_;
    std::string broadcasterId_;
    
    // Prediction state
    bool initialized_ = false;
    bool predictionActive_ = false;
    std::string currentPredictionId_;
    std::string outcomeWinId_;
    std::string outcomeLoseId_;
};