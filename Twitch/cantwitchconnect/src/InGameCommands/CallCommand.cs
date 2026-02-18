using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using TwitchLib.Client;
using Vintagestory.API.Common;
using Vintagestory.API.Config;
using Vintagestory.API.Server;

namespace cantwitchconnect.src.InGameCommands
{
    public class CallCommand : IngameCommand
    {
        public string[] PlayerNames;
        public string CommandToCall;
        string[] allowedList = ["time"];
        public CallCommand(TwitchClient client, ICoreServerAPI sapi, Config config, string name, Dictionary<string, object> paramDict) : base(client, sapi, config, name, paramDict)
        {
            if(paramDict.TryGetValue("PlayerNames", out var li))
            {
                PlayerNames = (li as JArray).ToObject<List<string>>().ToArray();
            }
            if (paramDict.TryGetValue("CommandToCall", out var commandStr))
            {
                CommandToCall = commandStr.ToString();
            }
        }
        public override void OnVotingFinished(int winner)
        {
            base.OnVotingFinished(winner);
            if (winner == 0)
            {   
                if(!allowedList.Contains(CommandToCall.Substring(1).Split(' ')[0]))
                {
                    return;
                }
                var c = new TextCommandCallingArgs();
                c.Caller = new Caller
                {
                    Type = EnumCallerType.Console,
                    CallerRole = "admin",
                    CallerPrivileges = new string[] { "*" },
                    FromChatGroupId = GlobalConstants.ConsoleGroup
                };
                sapi.ChatCommands.ExecuteUnparsed(CommandToCall, c);
            }              
        }
    }
}
