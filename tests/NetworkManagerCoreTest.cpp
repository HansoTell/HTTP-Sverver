#include "Error/Errorcodes.h"
#include "http/NetworkManager.h"
#include "http/listener.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sys/types.h>
#include <utility>


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
};

class MOCKListener : public http::Listener{
public:
    MOCK_METHOD(http::Result<http::SocketHandlers>, initSocket, (u_int16_t), (override));
    MOCK_METHOD(void, startListening, (), (override));
    MOCK_METHOD(void, stopListening, (), (override));

    MOCK_METHOD(http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>>*, getErrorQueue, (), (override) );
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getReceivedQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getOutgoingQueue, (), (override));
};

class MOCKListenerFactory : public http::IListenerFactory {
    MOCK_METHOD(std::unique_ptr<http::IListener>, createListener, (), (override));
};

class NetworkManagerCoreTest : public ::testing::Test {
protected:
    std::shared_ptr<MOCKSteamNetworkingSockets> mockSteam;
    std::unique_ptr<MOCKListenerFactory> mockFactory;
    http::NetworkManagerCore* manager;
     
    void SetUp() override {
        mockFactory = std::make_unique<MOCKListenerFactory>();
        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();

        manager = new http::NetworkManagerCore( mockSteam, std::move(mockFactory) );
    }

    void TearDown() override {
        delete manager;
    }
};
