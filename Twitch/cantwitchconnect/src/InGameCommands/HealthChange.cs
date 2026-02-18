using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;
using TwitchLib.Client;
using Vintagestory.API.Config;
using Vintagestory.API.Server;
using Vintagestory.GameContent;

namespace cantwitchconnect.src.InGameCommands
{
    public class HealthChange : IngameCommand
    {
        public enum HealthChangeType
        {
            RESTORE_FULL, RESTORE_HALF, REMOVE_HALF, RESTORE_NUMBER, REMOVE_NUMBER, SET_HALF
        }
        public string[] PlayerNames;
        public HealthChangeType healthChangeType;
        public HealthChange(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("healthChangeType", out var healthChangeTypeStr))
            {
                healthChangeType = (HealthChangeType)Enum.Parse(typeof(HealthChangeType), healthChangeTypeStr.ToString());
            }
            else
            {
                this.healthChangeType = HealthChangeType.REMOVE_HALF;
            }    
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            if (winner == 0)
            {
                foreach (string player in PlayerNames)
                {
                    foreach (var it in sapi.World.AllOnlinePlayers)
                    {
                        if (it.PlayerName == player)
                        {
                            if (this.healthChangeType == HealthChangeType.RESTORE_FULL)
                            {
                                it.Entity.GetBehavior<EntityBehaviorHealth>().Health = it.Entity.GetBehavior<EntityBehaviorHealth>().MaxHealth;
                            }
                            else if(this.healthChangeType == HealthChangeType.RESTORE_HALF)
                            {
                                it.Entity.GetBehavior<EntityBehaviorHealth>().Health += it.Entity.GetBehavior<EntityBehaviorHealth>().MaxHealth / 2;
                            }
                            else if (this.healthChangeType == HealthChangeType.REMOVE_HALF)
                            {
                                it.Entity.GetBehavior<EntityBehaviorHealth>().Health -= it.Entity.GetBehavior<EntityBehaviorHealth>().MaxHealth / 2;
                            }
                            else if (this.healthChangeType == HealthChangeType.SET_HALF)
                            {
                                it.Entity.GetBehavior<EntityBehaviorHealth>().Health = it.Entity.GetBehavior<EntityBehaviorHealth>().MaxHealth / 2;
                            }
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
