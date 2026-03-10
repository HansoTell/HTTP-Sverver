#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
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
    MOCKSteamNetworkingSockets* pMockSteam;
    std::unique_ptr<MOCKListenerFactory> mockFactory;
    MOCKListenerFactory* pMockFactory;
    http::NetworkManagerCore* manager;
     
    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        mockFactory = std::make_unique<MOCKListenerFactory>();
        pMockFactory = mockFactory.get();
        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();


        manager = new http::NetworkManagerCore( mockSteam, std::move(mockFactory) );
    }

    void TearDown() override {
        delete manager;
        DESTROY_LOGGER();
    }

    void setupListener(MOCKListener*& outListener){
        auto MockListener = std::make_unique<MOCKListener>();
        outListener = MockListener.get();

        EXPECT_CALL(*pMockFactory, createListener()).WillOnce(Return(ByMove(std::move(MockListener))));
    }

    HListener setupListeningState(u_int16_t port, MOCKListener*& outListener){
        setupListener(outListener);
        
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

//Create Listener
TEST_F(NetworkManagerCoreTest, CreateListener_Success){
    MOCKListener* pListener = nullptr;

    setupListener(pListener);

    HListener handler = manager->createListener("Test");


    EXPECT_NE(handler, HListener_Invalid);
    EXPECT_TRUE(manager->m_SocketClientsMap.empty());
    //Gibt es eine möglichkeit auf m_Listener zu zugreifen um größe, name zu prüfen und ptr zu prüfen in Listener Info

}

//DestroyListener
TEST_F(NetworkManagerCoreTest, DestroyListener_Success){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");

    auto res = manager->DestroyListener(handler);

    //wie manager sachen +berprüfen von m<-listener
    EXPECT_EQ(manager->m_SocketClientsMap.size(), 0);
    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerCoreTest, Destroy_Listener_invalidListener){
    auto res = manager->DestroyListener(1234);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, DestroyListenerInListeningState){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    auto res = manager->DestroyListener(handler);

    //m_Listener scheiß??
    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerCoreTest, StartListening_Success){
    MOCKListener* pListener = nullptr;

    setupListener(pListener);

    HListener handler = manager->createListener("Test");

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = 12345;
    fakeHandlers.m_PollGroup = 55;

    EXPECT_CALL(*pListener, initSocket(8080)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).Times(1);

    auto result = manager->startListening(handler, 8080);


    EXPECT_TRUE(result.isOK());
    EXPECT_TRUE(manager->m_SocketClientsMap.find(12345)!= manager->m_SocketClientsMap.end());

    EXPECT_EQ(manager->m_SocketClientsMap.at(12345).m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, StartListening_initSocketFails){
    MOCKListener* pListener = nullptr;

    setupListener(pListener);

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

    EXPECT_CALL(*pListener, initSocket(_)).Times(0);
    EXPECT_CALL(*pListener, startListening()).Times(0);

    auto result = manager->startListening(handler, 9000);

    ASSERT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}


//stopListening

TEST_F(NetworkManagerCoreTest, StopListening_invalidHandler){
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);

    auto res = manager->stopListening(1234);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, StopListeningWithoutListening){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");

    EXPECT_CALL(*pListener, stopListening()).Times(0);
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);

    auto res = manager->stopListening(handler);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, StopListening_Success){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    #define CONNECTIONS_NUM 3
    for(u_int32_t i = 0; i < CONNECTIONS_NUM; i++){
        manager->m_SocketClientsMap.at(12345).m_AllConnections.push_back( { i, false } );
    }
    manager->m_SocketClientsMap.at(12345).m_AllConnections.push_back( { 222, true } );

    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(CONNECTIONS_NUM+1);
    
    //Falls darin noch was passiert hier testen

    EXPECT_CALL(*pListener, stopListening()).Times(1);

    manager->stopListening(handler);

    EXPECT_TRUE(manager->m_SocketClientsMap.empty());
}
