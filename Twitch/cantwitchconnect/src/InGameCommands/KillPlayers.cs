using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TwitchLib.Client;
using TwitchLib.Client.Interfaces;
using Vintagestory.API.Config;
using Vintagestory.API.Server;
using Vintagestory.GameContent;

namespace cantwitchconnect.src.InGameCommands
{
    public class KillPlayers: IngameCommand
    {
        public string[] PlayerNames;
        public KillPlayers(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            if(winner == 0)
            {
                foreach (string player in PlayerNames)
                {
                    foreach (var it in sapi.World.AllOnlinePlayers)
                    {
                        if (it.PlayerName == player)
                        {
                            it.Entity.Die();
                        }
                    }
                }
            }
            else
            {
                //spare for now
            }
        }
    }
}
