#pragma once

#include "http/HTTPinitialization.h"
#include "steam/steamnetworkingtypes.h"

#include "gmock/gmock.h"
#include <cstring>
#include <gmock/gmock.h>
#include <vector>


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

class FakeSteamNetowrkingMessage : public SteamNetworkingMessage_t {
public:
    std::vector<char> buff;

    FakeSteamNetowrkingMessage(const char* msg, HSteamNetConnection con){
        buff.assign(msg, msg + strlen(msg));

        m_pData = buff.data();
        m_cbSize = buff.size();
        m_conn = con;

        m_pfnRelease = [](SteamNetworkingMessage_t* msg){
            delete static_cast<FakeSteamNetowrkingMessage*>(msg);
        };

        m_pfnFreeData = nullptr;
    }

    ~FakeSteamNetowrkingMessage() = default;
};
