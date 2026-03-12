#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "http/listener.h"
#include "gmock/gmock.h"
#include <algorithm>
#include <functional>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <sys/types.h>
#include <utility>
#include <vector>

#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "mocks/ListenerMock.h"
#include "steam/steamclientpublic.h"
#include "steam/steamnetworkingtypes.h"

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

    int conCount = 1;
     
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

    //geben wir connection number zurück? oder nehmen wir sie als parameter rein
    HSteamNetConnection addConnection( HSteamListenSocket socket){
        HSteamNetConnection con = conCount;

        SteamNetConnectionStatusChangedCallback_t info{};
        info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
        info.m_hConn = con;
        info.m_info.m_hListenSocket = socket;

        EXPECT_CALL(*pMockSteam, AcceptConnection(con)).WillOnce(Return(k_EResultOK));
        EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
        EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, 55)).WillOnce(Return(1));

        manager->callbackManager(&info);

        auto& socketInfo = manager->getSocketClientsMap().at(12345);

        EXPECT_EQ(socketInfo.m_AllConnections.size(), con);

        conCount++;

        return con;
    }
};

//Create Listener
TEST_F(NetworkManagerCoreTest, CreateListener_Success){
    MOCKListener* pListener = nullptr;

    setupListener(pListener);

    HListener handler = manager->createListener("Test");


    EXPECT_NE(handler, HListener_Invalid);
    EXPECT_TRUE(manager->isSocketClientsMapEmpty());
    EXPECT_STREQ(manager->getListenerMap().at(handler).ListenerName, "Test");
    EXPECT_EQ(manager->getListenerMap().at(handler).m_Socket, k_HSteamListenSocket_Invalid);
    EXPECT_EQ(manager->getListenerMap().at(handler).m_Listener.get(), pListener);
}

//DestroyListener
TEST_F(NetworkManagerCoreTest, DestroyListener_Success){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");

    auto res = manager->DestroyListener(handler);

    EXPECT_EQ(manager->getSocketClientsMap().size(), 0);
    EXPECT_TRUE(manager->getListenerMap().empty());
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

    EXPECT_CALL(*pListener, stopListening()).Times(1);
    auto res = manager->DestroyListener(handler);

    EXPECT_TRUE(manager->getListenerMap().empty());
    EXPECT_TRUE(res.isOK());
}

//Start Listening
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
    ASSERT_TRUE(manager->getSocketClientsMap().find(12345) != manager->getSocketClientsMap().end());
    EXPECT_EQ(manager->getSocketClientsMap().at(12345).m_AllConnections.size(), 0);
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

//callbackManager
TEST_F(NetworkManagerCoreTest, callbackManager_successConnecting){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = 222;
    info.m_info.m_hListenSocket = 12345;

    EXPECT_CALL(*pMockSteam, AcceptConnection(222)).WillOnce(Return(k_EResultOK));
    EXPECT_CALL(*pMockSteam, CloseConnection(222, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(222, 55)).WillOnce(Return(1));

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(12345);

    EXPECT_EQ(socketInfo.m_PollGroup, 55);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, 222);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
}

TEST_F(NetworkManagerCoreTest, callbackManager_ConnectingAcceptConnectionFailes){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = 222;

    EXPECT_CALL(*pMockSteam, AcceptConnection(222)).WillOnce(Return(k_EResultRemoteCallFailed));
    EXPECT_CALL(*pMockSteam, CloseConnection(222, 0, nullptr, false)).Times(1);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(222, 55)).Times(0);


    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(12345);

    EXPECT_EQ(socketInfo.m_PollGroup, 55);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_SettingPollGroupFailes){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = 222;
    info.m_info.m_hListenSocket = 12345;

    EXPECT_CALL(*pMockSteam, AcceptConnection(222)).WillOnce(Return(k_EResultOK));
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(222, 55)).WillOnce(Return(0));
    EXPECT_CALL(*pMockSteam, CloseConnection(222, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(12345);

    EXPECT_EQ(socketInfo.m_PollGroup, 55);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_problemIntern_Success){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    HSteamNetConnection con = addConnection(12345);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
    info.m_eOldState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = con;
    info.m_info.m_hListenSocket = 12345;
    strcpy(info.m_info.m_szConnectionDescription, "test_connection");

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& allClients = manager->getSocketClientsMap().at(12345).m_AllConnections;
    EXPECT_EQ(allClients.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_ClosedByPeerSuccess){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    HSteamNetConnection con = addConnection(12345);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_ClosedByPeer;
    info.m_eOldState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = con;
    info.m_info.m_hListenSocket = 12345;
    strcpy(info.m_info.m_szConnectionDescription, "test_connection");

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& allClients = manager->getSocketClientsMap().at(12345).m_AllConnections;
    EXPECT_EQ(allClients.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_unknownEnum){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    HSteamNetConnection con = addConnection(12345);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState__Force32Bit;

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, AcceptConnection(con)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, 55)).Times(0);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(12345);

    EXPECT_EQ(socketInfo.m_PollGroup, 55);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, con);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
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
    std::vector<HSteamNetConnection> cons;

    #define CONNECTIONS_NUM 4
    for(u_int32_t i = 0; i < CONNECTIONS_NUM; i++){
        HSteamNetConnection con = addConnection(12345);
        cons.push_back(con);
    }

    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(CONNECTIONS_NUM);
    
    //Falls darin noch was passiert hier testen

    EXPECT_CALL(*pListener, stopListening()).Times(1);

    manager->stopListening(handler);

    EXPECT_TRUE(manager->isSocketClientsMapEmpty());
}

TEST_F(NetworkManagerCoreTest, StopListening_DoubleCall){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);
    EXPECT_CALL(*pListener, stopListening()).Times(1);
    manager->stopListening(handler);

    EXPECT_CALL(*pListener, stopListening()).Times(0);

    auto res = manager->stopListening(handler);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, StopListening_RestartListening){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);
    EXPECT_CALL(*pListener, stopListening()).Times(1);
    manager->stopListening(handler);

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = 15;
    fakeHandlers.m_PollGroup = 55;

    EXPECT_CALL(*pListener, initSocket(8080)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).Times(1);

    auto res = manager->startListening(handler, 8080);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(manager->getSocketClientsMap().find(15) != manager->getSocketClientsMap().end());
    EXPECT_EQ(manager->getSocketClientsMap().at(15).m_AllConnections.size(), 0);
}

//getQueue
TEST_F(NetworkManagerCoreTest, getQueue_InvalidListener){
    auto res = manager->getQueue(123, http::QueueType::OUTGOING);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, getQueue_Success_Outgoing){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");
    
    EXPECT_CALL(*pListener, getOutgoingQueue()).WillOnce(Return(nullptr));
    auto res = manager->getQueue(handler, http::QueueType::OUTGOING);
    
    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerCoreTest, getQueue_Success_Received){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");
    
    EXPECT_CALL(*pListener, getReceivedQueue()).WillOnce(Return(nullptr));
    auto res = manager->getQueue(handler, http::QueueType::RECEIVED);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerCoreTest, getErrorQueue_InvalidListener){
    auto res = manager->getErrorQueue(123);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, getErrorQueue_Success){
    MOCKListener* pListener = nullptr;
    setupListener(pListener);
    HListener handler = manager->createListener("Test");
    
    EXPECT_CALL(*pListener, getErrorQueue()).WillOnce(Return(nullptr));
    auto res = manager->getErrorQueue(handler);
    
    EXPECT_TRUE(res.isOK());
}

//ConnectionServed
TEST_F(NetworkManagerCoreTest, ConnectionServed_Success){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    HSteamNetConnection con = addConnection(12345);

    manager->ConnectionServed(12345, con);

    auto& allClients = manager->getSocketClientsMap().at(12345).m_AllConnections;
    auto it = std::find_if(allClients.begin(), allClients.end(), [con](const http::Connections& conns){
        return conns.m_connection == con;
    });

    EXPECT_TRUE(it->isServed);
}

TEST_F(NetworkManagerCoreTest, ConnectionServed_DoubleServed){
    MOCKListener* pListener = nullptr;
    HListener handler = setupListeningState(8080, pListener);

    HSteamNetConnection con = addConnection(12345);

    manager->ConnectionServed(12345, con);

    auto& allClients = manager->getSocketClientsMap().at(12345).m_AllConnections;
    auto it = std::find_if(allClients.begin(), allClients.end(), [con](const http::Connections& conns){
        return conns.m_connection == con;
    });

    EXPECT_TRUE(it->isServed);
}

//PollConnectionChanges
TEST_F(NetworkManagerCoreTest, PollConnectionChanges){
    EXPECT_CALL(*pMockSteam, RunCallbacks()).Times(1);

    manager->pollConnectionChanges();
}

//PollFunctionCalls
TEST_F(NetworkManagerCoreTest, pollFunctionCalls_EmptyQueue){
    http::ThreadSaveQueue<std::function<void()>> functionCall;


    EXPECT_NO_THROW(manager->pollFunctionCalls(&functionCall));
    EXPECT_TRUE(functionCall.empty());
}

TEST_F(NetworkManagerCoreTest, pollFunctionCalls_overMessageLimit){
    http::ThreadSaveQueue<std::function<void()>> functionCall;
    int counter = 0;

    for(int i = 0; i < 30; i++){
        functionCall.push([&counter](){
            counter++;
        });
    }

    manager->pollFunctionCalls(&functionCall);

    EXPECT_EQ(counter, 20);
    EXPECT_EQ(functionCall.size(), 10);
}

TEST_F(NetworkManagerCoreTest, pollFunctionCalls_Success){
    http::ThreadSaveQueue<std::function<void()>> functionCall;
    int counter = 0;

    for(int i = 0; i < 10; i++){
        functionCall.push([&counter](){
            counter++;
        });
    }

    manager->pollFunctionCalls(&functionCall);

    EXPECT_EQ(counter, 10);
    EXPECT_EQ(functionCall.size(), 0);
}

