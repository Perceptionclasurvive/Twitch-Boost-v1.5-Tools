using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Text;
using TwitchLib.Api.Helix.Models.Charity;
using TwitchLib.Client;
using Vintagestory.API.Common;
using Vintagestory.API.Common.Entities;
using Vintagestory.API.Config;
using Vintagestory.API.Server;

namespace cantwitchconnect.src.InGameCommands
{
    public class SetOnfire : IngameCommand
    {
        public string[] PlayerNames;
        public SetOnfire(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
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
                            it.Entity.IsOnFire = true;
                        }
                    }
                }
            }              
        }

        public override void OnVotingStart()
        {
            this.client.SendMessageAsync(config.Channel, this.startCode);
            var sb = new StringBuilder();
            foreach(var it in this.answerInfos)
            {
                sb.Append(Lang.Get(it.LangCode)).Append(string.Join(',', it.Aliases));
                sb.AppendLine();
            }
            this.client.SendMessageAsync(config.Channel, sb.ToString());
        }
    }
}
