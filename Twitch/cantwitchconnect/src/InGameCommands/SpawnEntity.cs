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
    public class SpawnEntity: IngameCommand
    {
        public string[] PlayerNames;
        public string[] EntityCodes;
        public SpawnEntity(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("EntityCodes", out var liEntities))
            {
                EntityCodes = (liEntities as JArray).ToObject<List<string>>().ToArray();
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            int amount = winner;
            List<EntityProperties> entitiesCodes = new List<EntityProperties>();
            foreach(var it in EntityCodes)
            {
                entitiesCodes.Add(this.sapi.World.GetEntityType(new AssetLocation(it)));
            }
            EntityProperties[] entityTypes = entitiesCodes.ToArray();

            foreach (string player in PlayerNames)
            {
                foreach (var it in sapi.World.AllOnlinePlayers)
                {
                    if (it.PlayerName == player)
                    {
                        for (int i = 0; i < amount; i++)
                        {
                            EntityProperties currentProperties = entityTypes[random.Next(entityTypes.Length - 1)];
                            Entity entity = this.sapi.World.ClassRegistry.CreateEntity(currentProperties);
                            if (entity == null)
                            {
                                continue;
                            }
                            entity.Pos.SetPos(it.Entity.Pos.X + this.random.Next(2), it.Entity.Pos.Y + this.random.Next(1), it.Entity.Pos.Z + this.random.Next(2));
                            this.sapi.World.SpawnEntity(entity);
                        }
                    }
                }
            }           
        }
    }
}
