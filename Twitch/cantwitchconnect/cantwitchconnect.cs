using cantwitchconnect.src;
using System;
using System.Threading.Tasks;
using TwitchLib.Api;
using TwitchLib.Client;
using TwitchLib.Client.Events;
using TwitchLib.Client.Models;
using Vintagestory.API.Common;
using Vintagestory.API.Server;

namespace cantwitchconnect
{
    public class cantwitchconnect : ModSystem
    {
        public static ICoreServerAPI sapi;
        public Config config;
        private PollsHandler pollsHandler;
        TwitchClient twitchClient;
        public override void StartServerSide(ICoreServerAPI api)
        {
            sapi = api;

            config = api.LoadModConfig<Config>(Mod.Info.ModID);
            if(config == null)
            {
                config = new();
                config.InitConfig();
            }
            api.StoreModConfig(config, Mod.Info.ModID);

            try
            {
                Task.Run(async () => await GetAcccessAndRefreshTokens(config.AccessCode)).Wait();
                var credentials = new ConnectionCredentials(config.Channel, "oauth:" + config.AccessToken);
                twitchClient = new TwitchClient();
                twitchClient.Initialize(credentials, config.Channel);
                Task.Run(async () => await StartBot()).Wait();
                twitchClient.OnConnected += Client_OnConnected;
                twitchClient.OnJoinedChannel += Client_OnJoinedChannel;
                var twitchAPI = new TwitchAPI();

                pollsHandler = new PollsHandler(config, sapi, twitchClient, twitchAPI);

                twitchAPI.Settings.ClientId = config.ClientId;
                twitchAPI.Settings.Secret = config.ClientSecret;
            }
            catch (Exception ex) 
            {
            //skip for now
            }
            
        }
        async Task StartBot()
        {
            await this.twitchClient.ConnectAsync();
        }
        async Task Client_OnJoinedChannel(object? sender, OnJoinedChannelArgs e)
        {
            //Console.WriteLine("joinedchannel");
            //await twitchClient.SendMessageAsync(config.Channel, "456");
        }
        async Task Client_OnConnected(object? sender, OnConnectedEventArgs e)
        {
            await twitchClient.JoinChannelAsync(config.Channel);
        }
        async Task<bool> GetAcccessAndRefreshTokens(string code)
        {
            var ta = new TwitchAPI();
            var tResponse = await ta.Auth.ValidateAccessTokenAsync(config.AccessToken);
            if(tResponse != null)
            {
                return true;
            }
            if(tResponse == null)
            {
                if (config.RefreshToken != null && config.RefreshToken != "")
                {
                    var refreshResponse = await ta.Auth.RefreshAuthTokenAsync(config.RefreshToken, config.ClientSecret, config.ClientId);
                    config.AccessToken = refreshResponse.AccessToken;
                    config.RefreshToken = refreshResponse.RefreshToken;
                    sapi.StoreModConfig(config, Mod.Info.ModID);
                    return true;
                }
            }
            
            var refreshResponse2 = await ta.Auth.GetAccessTokenFromCodeAsync(code, config.ClientSecret, config.RedirectUri, config.ClientId);
            config.AccessToken = refreshResponse2.AccessToken;
            config.RefreshToken = refreshResponse2.RefreshToken;
            sapi.StoreModConfig(config, Mod.Info.ModID);
            return true;
        }
    }
}
