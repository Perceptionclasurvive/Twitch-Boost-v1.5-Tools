using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Text;
using TwitchLib.Api.Helix.Models.Charity;
using TwitchLib.Client;
using Vintagestory.API.Common;
using Vintagestory.API.Common.Entities;
using Vintagestory.API.Config;
using Vintagestory.API.Server;
using Vintagestory.Client.NoObf;
using Vintagestory.GameContent;
using Vintagestory.Server;
using static cantwitchconnect.src.InGameCommands.HealthChange;

namespace cantwitchconnect.src.InGameCommands
{
    public class ChangeTimeCommand : IngameCommand
    {
        public string[] PlayerNames;
        public TimeChangeType timeChangeType;
        public enum TimeChangeType
        {
            DAY, NIGHT
        }
        public ChangeTimeCommand(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("weatherChangeType", out var weatherChangeTypeStr))
            {
                timeChangeType = (TimeChangeType)Enum.Parse(typeof(TimeChangeType), weatherChangeTypeStr.ToString());
            }
            else
            {
                this.timeChangeType = TimeChangeType.DAY;
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            if (winner == 0)
            {
                if(this.timeChangeType == TimeChangeType.DAY)
                {
                    //(this.sapi.World as ServerMain).GameWorldCalendar
                    WeatherSystemServer wsysServer = this.sapi.ModLoader.GetModSystem<WeatherSystemServer>(true);
                    wsysServer.OverridePrecipitation = new float?(1);
                    wsysServer.serverChannel.BroadcastPacket<WeatherConfigPacket>(new WeatherConfigPacket
                    {
                        OverridePrecipitation = wsysServer.OverridePrecipitation,
                        RainCloudDaysOffset = wsysServer.RainCloudDaysOffset
                    }, Array.Empty<IServerPlayer>());
                }
                /*var args = new TextCommandCallingArgs()
                { Caller = new Caller()};*/
                var c = new Caller() { };
                //var pl = new ServerPlayer();
                sapi.ChatCommands.ExecuteUnparsed("/time set 12", null);
                /*(sapi as ICoreServerAPI)?.ChatCommands.ExecuteUnparsed(
                this.CommandToCall,
                new TextCommandCallingArgs() { Caller = c}*/
            //);
               // sapi.ChatCommands.Execute("weather", args);
                //sapi.ModLoader.GetModSystem<WeatherSystemBase>(true);
            }              
        }
    }
}
