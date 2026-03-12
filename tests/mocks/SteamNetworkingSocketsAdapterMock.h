#pragma once

#include "http/HTTPinitialization.h"
#include "steam/steamnetworkingtypes.h"

#include "gmock/gmock.h"
#include <gmock/gmock.h>


class MOCKSteamNetworkingSockets : public http::ISteamNetworkinSocketsAdapter  { 
public:
    MOCK_METHOD(EResult, AcceptConnection, (HSteamNetConnection), (override));
    MOCK_METHOD(bool, CloseConnection, ( HSteamNetConnection, int, const char *, bool), (override));
    MOCK_METHOD(bool, SetConnectionPollGroup, ( HSteamNetConnection, HSteamNetPollGroup ), (override));
    MOCK_METHOD(void, RunCallbacks, (), (override));

    MOCK_METHOD(HSteamNetPollGroup, CreatePollGroup, (), (override));
    MOCK_METHOD(HSteamListenSocket, CreateListenSocketIP, (const SteamNetworkingIPAddr&, int, const SteamNetworkingConfigValue_t*), (override));
    MOCK_METHOD(int, ReceiveMessagesOnPollGroup, (HSteamNetPollGroup, SteamNetworkingMessage_t **, int), (override));
    MOCK_METHOD(EResult, SendMessageToConnection, (HSteamNetConnection, const void*, uint32, int, int64*), (override));
    MOCK_METHOD(bool, CloseListenSocket, (HSteamListenSocket hSocket), (override));
    MOCK_METHOD(bool, DestroyPollGroup, (HSteamNetPollGroup), (override));
    MOCK_METHOD(bool, SetGlobalCallback_SteamNetConnectionStatusChanged, (FnSteamNetConnectionStatusChanged), (override));
};
