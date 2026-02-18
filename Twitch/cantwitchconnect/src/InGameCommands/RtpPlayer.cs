using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Text;
using TwitchLib.Client;
using Vintagestory.API.Common;
using Vintagestory.API.Common.Entities;
using Vintagestory.API.Config;
using Vintagestory.API.Server;
using Vintagestory.Server;

namespace cantwitchconnect.src.InGameCommands
{
    public class RtpPlayer : IngameCommand
    {
        public string[] PlayerNames;
        public int[] Radius;
        public RtpPlayer(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("Radius", out var liEntities))
            {
                Radius = (liEntities as JArray).ToObject<List<int>>().ToArray();
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            int amount = this.Radius[winner - 1];

            foreach (string player in PlayerNames)
            {
                foreach (var it in sapi.World.AllOnlinePlayers)
                {
                    if (it.PlayerName == player)
                    {
                        int x = this.random.Next(amount);
                        int z = this.random.Next(amount);
                        int y = this.sapi.WorldManager.GetSurfacePosY(x, z).Value;
                        y = 120; // for now
                        //y = this.sapi.World.BlockAccessor.GetTerrainMapheightAt(new Vintagestory.API.MathTools.BlockPos(x, 0, z));
                        //var c = (this.sapi.World as ServerMain).WorldMap.GetTerrainGenSurfacePosY(x, z);
                        it.Entity.TeleportToDouble(x, y, z);
                    }
                }
            }           
        }
    }
}
