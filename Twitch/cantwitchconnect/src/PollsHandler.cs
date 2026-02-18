using cantwitchconnect.src.InGameCommands;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using TwitchLib.Api;
using TwitchLib.Client;
using TwitchLib.Client.Events;
using Vintagestory.API.Server;

namespace cantwitchconnect.src
{
    public class PollsHandler
    {
        public ICoreServerAPI sapi;
        TwitchClient client;
        public Config config;
        public TwitchAPI twitchAPI;
        public Dictionary<string, string> commandNamesMapping;
        private readonly object voteLock = new object();
        public bool voteInProcess;
        public IngameCommand currentCommand;
        public ConcurrentDictionary<string, IngameCommand> inGameCommands = new();
        public ConcurrentDictionary<string, int> votes = new();
        public DateTime currentVotingEnd;
        
        public PollsHandler(Config config, ICoreServerAPI sapi, TwitchClient twitchClient, TwitchAPI twitchApi)
        {
            this.sapi = sapi;
            this.client = twitchClient;
            this.config = config;
            this.twitchAPI = twitchApi;
            votes = new();
            commandNamesMapping = new Dictionary<string, string>() { 
                { "kill", "killplayers" },
                { "fullhealth", "fullhealth" },
                { "wolfs", "spawnwolfs"},
                { "bear", "spawnbear" },
                { "toss", "tossup" },
                { "setonfire", "setonfire" },
                { "rtp", "rtp" }, 
                { "drifters", "spawndrifters" },
                { "setday", "setday" },
                { "setnight", "setnight" },
                { "stoprain", "stoprain" },
                { "startrain", "startrain"},
                { "halfhealth", "halfhealth" }
            };
            twitchClient.OnConnected += (s, e) =>
            {
                client.OnMessageReceived += Client_OnChatMessageReceived;
                return Task.CompletedTask;
            };

            List<IngameCommand> commands = new List<IngameCommand>
            {   
                new KillPlayers(client, sapi, config, "kill", config.InGameCommandsConfigs["killplayers"]),
                new HealthChange(client, sapi, config, "fullhealth", config.InGameCommandsConfigs["fullhealth"]),
                new HealthChange(client, sapi, config, "halfhealth", config.InGameCommandsConfigs["halfhealth"]),
                new SpawnEntity(client, sapi, config, "wolves", config.InGameCommandsConfigs["spawnwolves"]),
                new SpawnEntity(client, sapi, config, "bear", config.InGameCommandsConfigs["spawnbear"]),
                new SpawnEntity(client, sapi, config, "drifters", config.InGameCommandsConfigs["spawndrifters"]),
                new TossPlayer(client, sapi, config, "toss", config.InGameCommandsConfigs["tossup"]),
                new SetOnfire(client, sapi, config, "setonfire", config.InGameCommandsConfigs["setonfire"]),
                new CallCommand(client, sapi, config, "setday", config.InGameCommandsConfigs["setday"]),
                new CallCommand(client, sapi, config, "setnight", config.InGameCommandsConfigs["setnight"]),
                new ChangeWeatherCommand(client, sapi, config, "stoprain", config.InGameCommandsConfigs["stoprain"]),
                new ChangeWeatherCommand(client, sapi, config, "startrain", config.InGameCommandsConfigs["startrain"]),
                new RtpPlayer(client, sapi, config, "rtp", config.InGameCommandsConfigs["rtpplayer"])
            };

            foreach(var it  in commands)
            {
                this.inGameCommands[it.Name] = it;
            }

            sapi.Event.Timer(() =>
            {
                if(DateTime.Now >  currentVotingEnd)
                {
                    if (voteInProcess)
                    {
                        lock (voteLock)
                        {
                            voteInProcess = false;                          
                        }
                        OnVotingFinished();
                    }
                }
            }, 1);
        }
        public Task Client_OnChatMessageReceived(object sender, OnMessageReceivedArgs e)
        {
            //this.client.JoinChannelAsync(config.Channel);
            //client.send
            if (this.voteInProcess && this.currentCommand != null && !votes.TryGetValue(e.ChatMessage.Username, out int voteVariant))
            {
                int selectedAnswer = currentCommand.TryToPlaceVote(e);
                votes[e.ChatMessage.Username] = selectedAnswer;
            }
            if (e.ChatMessage.Message.StartsWith("!"))
            {
                if (this.voteInProcess)
                {
                    return Task.CompletedTask;
                }
                string command = e.ChatMessage.Message.Substring(1).Split(' ').FirstOrDefault("");
                if (!this.commandNamesMapping.TryGetValue(command, out string className))
                {
                    return Task.CompletedTask;
                }
                lock (voteLock)
                {
                    if (voteInProcess)
                    {
                        return Task.CompletedTask;
                    }
                    if(this.inGameCommands.TryGetValue(command, out var inGameCommand))
                    {
                        this.currentCommand = inGameCommand;
                        voteInProcess = true;
                        this.currentVotingEnd = DateTime.Now.AddSeconds(this.currentCommand.secondsForVote);
                        this.currentCommand.OnVotingStart();
                    }
                    else
                    {

                    }
                        
                }
            }
            return Task.CompletedTask;
        }
        public void OnVotingFinished()
        {
            if(currentCommand == null)
            {
                return;
            }
            if (this.votes.Count >= this.currentCommand.minimumAmountOfVotes)
            {
                int mostFrequent = this.votes
                    .GroupBy(kv => kv.Value)
                    .OrderByDescending(g => g.Count())
                    .First()
                    .Key;
                this.votes.Clear();
                this.currentCommand.OnVotingFinished(mostFrequent);
            }
        }
    } 
}
