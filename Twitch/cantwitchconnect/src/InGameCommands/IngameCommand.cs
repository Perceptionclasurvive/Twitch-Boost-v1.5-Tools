using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using TwitchLib.Client;
using TwitchLib.Client.Events;
using Vintagestory.API.Config;
using Vintagestory.API.Server;

namespace cantwitchconnect.src.InGameCommands
{
    public abstract class IngameCommand
    {
        public class AnswerInfo
        {
            public string AnswerName;
            public string LangCode;
            public string[] Aliases;
            public AnswerInfo(string answerName, string langCode, string[] aliases)
            {
                AnswerName = answerName;
                LangCode = langCode;
                Aliases = aliases;
            }
        }
        protected TwitchClient client;
        public Config config;
        public ICoreServerAPI sapi;
        public string Name;
        public AnswerInfo[] answerInfos;
        public int secondsForVote;
        public string startCode;
        public string finishCode;
        public int minimumAmountOfVotes;
        protected Random random;
        public bool enabled = false;
        public IngameCommand(TwitchClient client, ICoreServerAPI sapi, Config config, string Name, Dictionary<string, object> paramDict)
        {
            this.client = client;
            this.config = config;
            this.sapi = sapi;
            this.Name = Name;
            random = new Random();
            if (paramDict.TryGetValue("enabled", out var enabledStr))
            {
                this.enabled = bool.Parse(enabledStr.ToString());
            }
            if (paramDict.TryGetValue("secondsForVote", out var secs))
            {
                this.secondsForVote = int.Parse(secs.ToString());
            }
            else
            {
                this.secondsForVote = 30;
            }
            if (paramDict.TryGetValue("answers", out var answers))
            {
                List<AnswerInfo> li = new();
                var c = (answers as Newtonsoft.Json.Linq.JArray).ToObject<AnswerInfo[]>();
                foreach (var answer in c)
                {
                    li.Add(answer);
                }
                this.answerInfos = li.ToArray();
            }            
            if(paramDict.TryGetValue("startCode", out var startCodeObject))
            {
                startCode = Lang.Get(startCodeObject.ToString(), this.Name);
            }
            else
            {
                startCode = Lang.Get("cantwitchconnect:poll_started_for_command", this.Name);
            }

            if (paramDict.TryGetValue("finishCode", out var finishCodeObject))
            {
                finishCode = finishCode.ToString();
            }
            else
            {
                finishCode = "cantwitchconnect:poll_finished_for_command";
            }

            if (paramDict.TryGetValue("minimumAmountOfVotes", out var minAmount))
            {
                this.minimumAmountOfVotes = int.Parse(minAmount.ToString());
            }
            else
            {
                this.minimumAmountOfVotes = 0;
            }

        }
        public int TryToPlaceVote(OnMessageReceivedArgs e)
        {
            string msg = e.ChatMessage.Message.Trim();
            int counter = 0;
            foreach (var info in answerInfos) 
            {
                if(info.Aliases.Contains(msg))
                {
                    return counter;
                }
                counter++;
            }
            return counter;
        }
        public virtual void OnVotingStart()
        {
            this.client.SendMessageAsync(config.Channel, this.startCode);
            var sb = new StringBuilder();
            foreach (var it in this.answerInfos)
            {
                sb.Append(Lang.Get(it.LangCode)).Append(": ").Append(string.Join(',', it.Aliases));
                sb.Append(" | ");
            }
            this.client.SendMessageAsync(config.Channel, sb.ToString());
        }
        public virtual void OnVotingFinished(int winner)
        {
            this.client.SendMessageAsync(config.Channel, Lang.Get(this.finishCode, this.Name, winner));
        }
    }
}
