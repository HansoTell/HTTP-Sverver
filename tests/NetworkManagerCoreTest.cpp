#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "http/listener.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sys/types.h>
#include <utility>

#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "mocks/ListenerMock.h"

using ::testing::Return;
using :: testing::ByMove;
using ::testing::_;


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

    HListener setupListeningState(u_int16_t port, MOCKListener*& outListener){
        auto mockListener = std::make_unique<MOCKListener>();
        outListener = mockListener.get();

        EXPECT_CALL(*mockFactory, createListener()).WillOnce(Return(ByMove(std::move(mockListener))));
        HListener handler = manager->createListener("Test");

        http::SocketHandlers fakeHandlers;
        fakeHandlers.m_Socket = 12345;
        fakeHandlers.m_PollGroup = 55;

        EXPECT_CALL(*outListener, initSocket(port)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
        EXPECT_CALL(*outListener, startListening()).Times(1);

        manager->startListening(handler, port);

        return handler;
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

    http::Result<http::SocketHandlers> errResult = MAKE_ERROR(http::HTTPErrors::eSocketInitializationFailed, "Init failed");

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

TEST_F(NetworkManagerCoreTest, StartListening_DoubleCall){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    //muss hier andere zahl weil soll ja genau einmal aufgerufen werden insegsammt...
    EXPECT_CALL(*pListener, initSocket(_)).Times(0);
    EXPECT_CALL(*pListener, startListening()).Times(0);

    auto result = manager->startListening(handler, 9000);

    ASSERT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}


//stopListening
TEST_F(NetworkManagerCoreTest, StopListening_Success){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    #define CONNECTIONS_NUM 3
    for(u_int32_t i = 0; i < CONNECTIONS_NUM; i++){
        manager->m_SocketClientsMap.at(12345).m_AllConnections.push_back( { i, false } );
    }
    manager->m_SocketClientsMap.at(12345).m_AllConnections.push_back( { 222, true } );

    EXPECT_CALL(*mockSteam, CloseConnection(_, 1, nullptr, false)).Times(CONNECTIONS_NUM) ;
    //Falls darin noch was passiert hier testen

    EXPECT_CALL(*pListener, stopListening()).Times(1);
    manager->stopListening(handler);

    EXPECT_TRUE(manager->m_SocketClientsMap.empty());
}







