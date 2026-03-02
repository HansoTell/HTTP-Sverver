#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "http/listener.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sys/types.h>
#include <utility>


using ::testing::Return;
using :: testing::ByMove;
using ::testing::_;


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

class MOCKListener : public http::IListener{
public:
    MOCK_METHOD(http::Result<http::SocketHandlers>, initSocket, (u_int16_t), (override));
    MOCK_METHOD(void, startListening, (), (override));
    MOCK_METHOD(void, stopListening, (), (override));

    MOCK_METHOD(http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>>*, getErrorQueue, (), (override) );
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getReceivedQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getOutgoingQueue, (), (override));
};

class MOCKListenerFactory : public http::IListenerFactory {
public:
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

TEST_F(NetworkManagerCoreTest, StartListening_Success){
    auto mockListener = std::make_unique<MOCKListener>();
    auto* pListener = mockListener.get();

    EXPECT_CALL(*mockFactory, createListener()).WillOnce(Return(ByMove(std::move(mockListener))));

    HListener handler = manager->createListener("Test");

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = 12345;
    fakeHandlers.m_PollGroup = 55;

    EXPECT_CALL(*pListener, initSocket(8080)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).Times(1);

    auto result = manager->startListening(handler, 8080);


    EXPECT_TRUE(result.isOK());
    EXPECT_TRUE(manager->m_SocketClientsMap.find(1234)!= manager->m_SocketClientsMap.end());
}

TEST_F(NetworkManagerCoreTest, StartListening_initSocketFails){
    auto mockListener = std::make_unique<MOCKListener>();
    auto* pListener = mockListener.get();

    EXPECT_CALL(*mockFactory, createListener()).WillOnce(Return(ByMove(std::move(mockListener))));

    HListener handler = manager->createListener("Test");

    http::Result<http::SocketHandlers> errResult = MAKE_ERROR(http::HTTPErrors::eInvalidSocket, "Init failed");

    EXPECT_CALL(*pListener, initSocket(8080)).WillOnce(Return(errResult));

    EXPECT_CALL(*pListener, startListening()).Times(0);

    auto result = manager->startListening(handler, 8080);

    EXPECT_TRUE(result.isErr());
}

TEST_F(NetworkManagerCoreTest, StartListening_invalidListener){

    auto result = manager->startListening(13, 8080);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}





