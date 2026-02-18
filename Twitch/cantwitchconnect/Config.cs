using System;
using System.Collections.Generic;
using static cantwitchconnect.src.InGameCommands.ChangeWeatherCommand;
using static cantwitchconnect.src.InGameCommands.HealthChange;
using static cantwitchconnect.src.InGameCommands.IngameCommand;

namespace cantwitchconnect
{
    public class Config
    {
        public string AccessCode;
        public string Channel;
        public string ClientId;
        public string ClientSecret;
        public string AccessToken;
        public string RefreshToken;
        public DateTime TokenExpiry;
        public string RedirectUri = "http://localhost";
        public string BroadcasterId;

        public Dictionary<string, Dictionary<string, object>> InGameCommandsConfigs;

        public void InitConfig()
        {
            this.AccessCode = "";
            this.Channel = "";
            this.ClientId = "";
            this.ClientSecret = "";
            this.RedirectUri = "http://localhost";
            this.InGameCommandsConfigs = new Dictionary<string, Dictionary<string, object>>()
            {             
                { "killplayers", new Dictionary<string, object>(){ 
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("killthem", "cantwitchconnect:killthem", new string[] { "kill", "0", "+", "yes" }),
                        new AnswerInfo("sparethem", "cantwitchconnect:sparethem", new string[] { "spare", "1", "-", "no" }) }
                    }  ,
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true }
                } },
                { "fullhealth", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("heal", "cantwitchconnect:healthem", ["0", "+", "yes"]),
                        new AnswerInfo("letbe", "cantwitchconnect:letbe", ["1", "-", "no"]) }
                    }  ,
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "healthChangeType", HealthChangeType.RESTORE_FULL }
                } },
                { "halfhealth", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("heal", "cantwitchconnect:healthem", ["0", "+", "yes"]),
                        new AnswerInfo("letbe", "cantwitchconnect:letbe", ["1", "-", "no"]) }
                    }  ,
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "healthChangeType", HealthChangeType.SET_HALF }
                } },
                { "spawnwolves", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("dontspawn", "cantwitchconnect:dontspawnwolves", ["0", "-", "no"]),
                        new AnswerInfo("spawn1", "cantwitchconnect:spawn1wolf", ["1"]),
                        new AnswerInfo("spawn2", "cantwitchconnect:spawn2wolf", ["2"]),
                        new AnswerInfo("spawn3", "cantwitchconnect:spawn1wolf", ["3"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "EntityCodes", new List<string>() { "game:wolf-male", "game:wolf-female" }  }
                } 
                },
                { "spawnbear", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("dontspawn", "cantwitchconnect:dontspawnbear", ["0", "-", "no"]),
                        new AnswerInfo("spawn1", "cantwitchconnect:spawn1bear", ["1"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "EntityCodes", new List<string>() { "game:bear-brown-adult-male", "game:bear-brown-adult-female" }  }
                }
                },
                { "spawndrifters", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("dontspawn", "cantwitchconnect:dontspawndrifters", ["0", "-", "no"]),
                        new AnswerInfo("spawn1", "cantwitchconnect:spawn1drifter", ["1"]),
                        new AnswerInfo("spawn2", "cantwitchconnect:spawn2drifter", ["2"]),
                        new AnswerInfo("spawn3", "cantwitchconnect:spawn3drifter", ["3"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "EntityCodes", new List<string>() { "game:drifter-normal", "game:drifter-deep", "game:drifter-tainted", "game:drifter-corrupt", "game:drifter-nightmare" }  }
                }
                },
                { "tossup", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("donttoss", "cantwitchconnect:donttoss", ["0", "-", "no"]),
                        new AnswerInfo("toss1", "cantwitchconnect:toss1", ["1"]),
                        new AnswerInfo("toss2", "cantwitchconnect:toss2", ["2"]),
                        new AnswerInfo("toss3", "cantwitchconnect:toss3", ["3"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "Heights", new List<int>() { 15, 30, 60}  }
                }
                },
                { "setonfire", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("setonfire", "cantwitchconnect:setonfire", ["0", "+", "yes"]),
                        new AnswerInfo("spare", "cantwitchconnect:spare", ["1", "-", "no"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "enabled", true },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 }
                }
                },
                { "setday", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("setday", "cantwitchconnect:setday", ["0", "+", "yes"]),
                        new AnswerInfo("dontchangetime", "cantwitchconnect:dontchangetime", ["1", "-", "no"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "CommandToCall", "/time set day" }
                }
                },
                { "setnight", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("setnight", "cantwitchconnect:setnight", ["0", "+", "yes"]),
                        new AnswerInfo("dontchangetime", "cantwitchconnect:dontchangetime", ["1", "-", "no"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "CommandToCall", "/time set night" }
                }
                },
                { "stoprain", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("stoprain", "cantwitchconnect:stoprain", ["0", "+", "yes"]),
                        new AnswerInfo("dontchangeweather", "cantwitchconnect:dontchangeweather", ["1", "-", "no"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "weatherChangeType", WeatherChangeType.STOP_RAIN }
                }
                },
                { "startrain", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("starrain", "cantwitchconnect:startrain", ["0", "+", "yes"]),
                        new AnswerInfo("dontchangeweather", "cantwitchconnect:dontchangeweather", ["1", "-", "no"])
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "weatherChangeType", WeatherChangeType.START_RAIN }
                }
                },
                { "rtpplayer", new Dictionary<string, object>(){
                    { "PlayerNames", new List<string>() { "KenigVovan" } },
                    { "answers", new List<AnswerInfo>{
                        new AnswerInfo("letthembe", "cantwitchconnect:letthembe", ["0", "-", "no"]),
                        new AnswerInfo("rtpplayer1", "cantwitchconnect:rtpplayer1", ["1"]),
                        new AnswerInfo("rtpplayer2", "cantwitchconnect:rtpplayer2", ["2"]),
                        new AnswerInfo("rtpplayer3", "cantwitchconnect:rtpplayer3", ["3"]),
                                                     }
                    },
                    { "secondsForVote", 30 },
                    { "startCode", "cantwitchconnect:poll_started_for_command" },
                    { "minimumAmountOfVotes", 1 },
                    { "enabled", true },
                    { "Radius", new List<int>() { 500, 1000, 1500 }  }
                }
                }
            };
        }
    }
}
