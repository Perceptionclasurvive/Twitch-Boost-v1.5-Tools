using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Text;
using TwitchLib.Client;
using Vintagestory.API.Common;
using Vintagestory.API.Common.Entities;
using Vintagestory.API.Config;
using Vintagestory.API.Server;

namespace cantwitchconnect.src.InGameCommands
{
    public class TossPlayer : IngameCommand
    {
        public string[] PlayerNames;
        public int[] Heights;
        public TossPlayer(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("Heights", out var liEntities))
            {
                Heights = (liEntities as JArray).ToObject<List<int>>().ToArray();
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            int amount = this.Heights[winner - 1];

            foreach (string player in PlayerNames)
            {
                foreach (var it in sapi.World.AllOnlinePlayers)
                {
                    if (it.PlayerName == player)
                    {
                         it.Entity.TeleportToDouble(it.Entity.Pos.X, it.Entity.Pos.Y + amount, it.Entity.Pos.Z);                       
                    }
                }
            }           
        }
    }
}
