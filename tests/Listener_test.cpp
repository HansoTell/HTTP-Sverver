#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

/*
class MockSteamNetworkingSockets : public ISteamNetworkingSockets {
public:
    MOCK_METHOD(HSteamListenSocket, CreateListenSocketIP, (const SteamNetworkingIPAddr&, int, const SteamNetworkingConfigValue_t*), (override));
    MOCK_METHOD(HSteamNetPollGroup, CreatePollGroup, (), (override));
    MOCK_METHOD(int, ReceiveMessagesOnPollGroup, (HSteamNetPollGroup, SteamNetworkingMessage_t**, int), (override));
    MOCK_METHOD(EResult, SendMessageToConnection, (HSteamNetConnection, const void*, uint32, int, int64*), (override) );
    MOCK_METHOD(bool, CloseListenSocket, (HSteamListenSocket), (override));
    MOCK_METHOD(bool, DestroyPollGroup, (HSteamNetPollGroup), (override));
    MOCK_METHOD(bool, CloseConnection, (HSteamNetConnection, int, const char*, bool), (override));
};

class ListenerTest : public ::testing::Test {
protected:
    std::unique_ptr<MockSteamNetworkingSockets> mock;
    std::unique_ptr<http::Listener> listener;

    void SetUp() override {
        mock = std::make_unique<MockSteamNetworkingSockets>();
        listener = std::make_unique<http::Listener>(mock.get());
    }
    void TearDown() override {
        listener->stopListening();
        listener.reset();
    } 

};
*/
