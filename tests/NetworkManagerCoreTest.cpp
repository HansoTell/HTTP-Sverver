#include "http/listener.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>


class MOCKSteamNetworkingSockets : public ISteamNetworkingSockets { 
public:
    MOCK_METHOD(EResult, AcceptConnection, (HSteamNetConnection), (override));
    MOCK_METHOD(bool, CloseConnection, ( HSteamNetConnection, int, const char *, bool), (override));
    MOCK_METHOD(bool, SetConnectionPollGroup, ( HSteamNetConnection, HSteamNetPollGroup ), (override));
    MOCK_METHOD(void, RunCallbacks, (), (override));
};

class MOCKListener : public http::Listener{
public:


};
