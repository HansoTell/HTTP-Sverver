#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Error.h"
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HTTPinitialization.h"
#include "http/NetworkManager.h"
#include "http/Request.h"
#include "http/listener.h"
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
#include "mocks/Test_Constants.h"
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
    MOCKListener* pListener;
    std::unique_ptr<http::NetworkManagerCore> manager;

    int conCount = 1;
     
    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        mockFactory = std::make_unique<MOCKListenerFactory>();
        pMockFactory = mockFactory.get();
        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();


        manager = std::make_unique<http::NetworkManagerCore>(mockSteam, std::move(mockFactory));
    }

    void TearDown() override {
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
        fakeHandlers.m_Socket = TEST_HSOCK;
        fakeHandlers.m_PollGroup = TEST_HPOLLGROUP;

        EXPECT_CALL(*outListener, initSocket(port)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
        EXPECT_CALL(*outListener, startListening()).Times(1);

        manager->startListening(handler, port);

        return handler;
    }

    HSteamNetConnection addConnection( HSteamListenSocket socket){
        HSteamNetConnection con = conCount;

        SteamNetConnectionStatusChangedCallback_t info{};
        info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
        info.m_hConn = con;
        info.m_info.m_hListenSocket = socket;

        EXPECT_CALL(*pMockSteam, AcceptConnection(con)).WillOnce(Return(k_EResultOK));
        EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
        EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, TEST_HPOLLGROUP)).WillOnce(Return(1));

        manager->callbackManager(&info);

        auto& socketInfo = manager->getSocketClientsMap().at(socket);

        EXPECT_EQ(socketInfo.m_AllConnections.size(), con);

        conCount++;

        return con;
    }
};

//Create Listener
TEST_F(NetworkManagerCoreTest, CreateListener_Success_ReturnValidHandler){
    setupListener(pListener);

    HListener handler = manager->createListener("Test");

    EXPECT_NE(handler, HListener_Invalid);
    EXPECT_TRUE(manager->isSocketClientsMapEmpty());
    EXPECT_STREQ(manager->getListenerMap().at(handler).ListenerName, "Test");
    EXPECT_EQ(manager->getListenerMap().at(handler).m_Socket, k_HSteamListenSocket_Invalid);
    EXPECT_EQ(manager->getListenerMap().at(handler).m_Listener.get(), pListener);
}

TEST_F(NetworkManagerCoreTest, Create_MultibleListeners_ReturnDiffrentHandlers){
    MOCKListener* pListener1 = nullptr;
    setupListener(pListener1);

    HListener handler1 = manager->createListener("Test");

    MOCKListener* pListener2 = nullptr;
    setupListener(pListener2);

    HListener handler2 = manager->createListener("Test1");

    EXPECT_NE(handler1, handler2);
    EXPECT_EQ(manager->getListenerMap().size(), 2);
}

//DestroyListener
TEST_F(NetworkManagerCoreTest, DestroyListener_Success_ReturnNoError){
    setupListener(pListener);
    HListener handler = manager->createListener("Test");

    auto res = manager->DestroyListener(handler);

    EXPECT_EQ(manager->getSocketClientsMap().size(), 0);
    EXPECT_TRUE(manager->getListenerMap().empty());
    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerCoreTest, DestroyListener_invalidListener_ReturnsError){
    auto res = manager->DestroyListener(TEST_HINVALIDLISTENER);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, DestroyListener_InListeningState_Succeedes){
    HListener handler = setupListeningState(TEST_PORT, pListener);

    EXPECT_CALL(*pListener, stopListening()).Times(1);
    auto res = manager->DestroyListener(handler);

    EXPECT_TRUE(manager->getListenerMap().empty());
    EXPECT_TRUE(res.isOK());
}

//Start Listening
TEST_F(NetworkManagerCoreTest, StartListening_Success_ReturnsNoError){
    setupListener(pListener);

    HListener handler = manager->createListener("Test");

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = TEST_HSOCK;
    fakeHandlers.m_PollGroup = TEST_HPOLLGROUP;

    EXPECT_CALL(*pListener, initSocket(TEST_PORT)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).WillOnce(Return(http::Result<void>()));

    auto result = manager->startListening(handler, TEST_PORT);

    EXPECT_TRUE(result.isOK());
    ASSERT_TRUE(manager->getSocketClientsMap().find(TEST_HSOCK) != manager->getSocketClientsMap().end());
    EXPECT_EQ(manager->getSocketClientsMap().at(TEST_HSOCK).m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, StartListening_initSocketFails_ReturnsError){
    setupListener(pListener);

    HListener handler = manager->createListener("Test");

    http::Result<http::SocketHandlers> errResult = MAKE_ERROR(http::HTTPErrors::eSocketInitializationFailed, "Init failed");

    EXPECT_CALL(*pListener, initSocket(TEST_PORT)).WillOnce(Return(errResult));

    EXPECT_CALL(*pListener, startListening()).Times(0);

    auto result = manager->startListening(handler, TEST_PORT);

    EXPECT_TRUE(result.isErr());
}

TEST_F(NetworkManagerCoreTest, StartListening_invalidListener_ReturnsError){
    auto result = manager->startListening(TEST_HINVALIDLISTENER, TEST_PORT);
    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, StartListening_DoubleCall_ReturnsError){
    HListener handler = setupListeningState(TEST_PORT, pListener);

    EXPECT_CALL(*pListener, initSocket(_)).Times(0);
    EXPECT_CALL(*pListener, startListening()).Times(0);

    auto result = manager->startListening(handler, 9000);

    ASSERT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, StartListening_ListenerStartListeningFails_RetunsError){
    setupListener(pListener);

    HListener handler = manager->createListener("Test");

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = TEST_HSOCK;
    fakeHandlers.m_PollGroup = TEST_HPOLLGROUP;

    EXPECT_CALL(*pListener, initSocket(TEST_PORT)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidCall, "Test")));

    auto result = manager->startListening(handler, TEST_PORT);

    EXPECT_TRUE(result.isErr());
    EXPECT_EQ(result.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

//callbackManager
TEST_F(NetworkManagerCoreTest, callbackManager_successConnecting_AddsConnection){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = TEST_HCONNECTION;
    info.m_info.m_hListenSocket = TEST_HSOCK;

    EXPECT_CALL(*pMockSteam, AcceptConnection(TEST_HCONNECTION)).WillOnce(Return(k_EResultOK));
    EXPECT_CALL(*pMockSteam, CloseConnection(TEST_HCONNECTION, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(TEST_HCONNECTION, TEST_HPOLLGROUP)).WillOnce(Return(1));

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, TEST_HCONNECTION);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
}

TEST_F(NetworkManagerCoreTest, callbackManager_ConnectingAcceptConnectionFailes_AddsNoConnection){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = TEST_HCONNECTION;

    EXPECT_CALL(*pMockSteam, AcceptConnection(TEST_HCONNECTION)).WillOnce(Return(k_EResultRemoteCallFailed));
    EXPECT_CALL(*pMockSteam, CloseConnection(TEST_HCONNECTION, 0, nullptr, false)).Times(1);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(TEST_HCONNECTION, TEST_HPOLLGROUP)).Times(0);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    EXPECT_EQ(socketInfo.m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_SettingPollGroupFailes_addsNoConnection){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = TEST_HCONNECTION;
    info.m_info.m_hListenSocket = TEST_HSOCK;

    EXPECT_CALL(*pMockSteam, AcceptConnection(TEST_HCONNECTION)).WillOnce(Return(k_EResultOK));
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(TEST_HCONNECTION, TEST_HPOLLGROUP)).WillOnce(Return(0));
    EXPECT_CALL(*pMockSteam, CloseConnection(TEST_HCONNECTION, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    EXPECT_EQ(socketInfo.m_AllConnections.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_problemInternSuccess_addsNoConnection){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_ProblemDetectedLocally;
    info.m_eOldState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = con;
    info.m_info.m_hListenSocket = TEST_HSOCK;
    strcpy(info.m_info.m_szConnectionDescription, "test_connection");

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& allClients = manager->getSocketClientsMap().at(TEST_HSOCK).m_AllConnections;
    EXPECT_EQ(allClients.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_ClosedByPeerSuccess_RemovesConnection){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_ClosedByPeer;
    info.m_eOldState = k_ESteamNetworkingConnectionState_Connecting;
    info.m_hConn = con;
    info.m_info.m_hListenSocket = TEST_HSOCK;
    strcpy(info.m_info.m_szConnectionDescription, "test_connection");

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(1);

    manager->callbackManager(&info);

    auto& allClients = manager->getSocketClientsMap().at(TEST_HSOCK).m_AllConnections;
    EXPECT_EQ(allClients.size(), 0);
}

TEST_F(NetworkManagerCoreTest, callbackManager_Connected_DoesNothing){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_Connected;

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, AcceptConnection(con)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, TEST_HPOLLGROUP)).Times(0);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, con);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
}


TEST_F(NetworkManagerCoreTest, callbackManager_none_DoesNothing){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState_None;

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, AcceptConnection(con)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, TEST_HPOLLGROUP)).Times(0);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, con);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
}

TEST_F(NetworkManagerCoreTest, callbackManager_unknownEnum_DoesNothing){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    SteamNetConnectionStatusChangedCallback_t info{};
    info.m_info.m_eState = k_ESteamNetworkingConnectionState__Force32Bit;

    EXPECT_CALL(*pMockSteam, CloseConnection(con, 0, nullptr, false)).Times(0);
    EXPECT_CALL(*pMockSteam, AcceptConnection(con)).Times(0);
    EXPECT_CALL(*pMockSteam, SetConnectionPollGroup(con, TEST_HPOLLGROUP)).Times(0);

    manager->callbackManager(&info);

    auto& socketInfo = manager->getSocketClientsMap().at(TEST_HSOCK);

    EXPECT_EQ(socketInfo.m_PollGroup, TEST_HPOLLGROUP);
    ASSERT_EQ(socketInfo.m_AllConnections.size(), 1);
    EXPECT_EQ(socketInfo.m_AllConnections.at(0).m_connection, con);
    EXPECT_FALSE(socketInfo.m_AllConnections[0].isServed);
}

//stopListening
TEST_F(NetworkManagerCoreTest, StopListening_invalidHandler_ReturnsError){
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);

    auto res = manager->stopListening(TEST_HINVALIDLISTENER);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, StopListening_WithoutListening_ReturnsError){
    setupListener(pListener);
    HListener handler = manager->createListener("Test");

    EXPECT_CALL(*pListener, stopListening()).Times(0);
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);

    auto res = manager->stopListening(handler);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, StopListening_Success_RemovesAllConnectinos){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    std::vector<HSteamNetConnection> cons;

    #define CONNECTIONS_NUM 4
    for(u_int32_t i = 0; i < CONNECTIONS_NUM; i++){
        HSteamNetConnection con = addConnection(TEST_HSOCK);
        cons.push_back(con);
    }

    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(CONNECTIONS_NUM);
    
    //Falls darin noch was passiert hier testen

    EXPECT_CALL(*pListener, stopListening()).Times(1);

    manager->stopListening(handler);

    EXPECT_TRUE(manager->isSocketClientsMapEmpty());
}

TEST_F(NetworkManagerCoreTest, StopListening_DoubleCall_ReturnsError){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    EXPECT_CALL(*pMockSteam, CloseConnection(_, 1, nullptr, false)).Times(0);
    EXPECT_CALL(*pListener, stopListening()).Times(1);
    manager->stopListening(handler);

    EXPECT_CALL(*pListener, stopListening()).Times(0);

    auto res = manager->stopListening(handler);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, StopListening_RestartListening_Succeedes){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    EXPECT_CALL(*pListener, stopListening()).Times(1);
    manager->stopListening(handler);

    http::SocketHandlers fakeHandlers;
    fakeHandlers.m_Socket = 15;
    fakeHandlers.m_PollGroup = TEST_HPOLLGROUP;

    EXPECT_CALL(*pListener, initSocket(TEST_PORT)).WillOnce(Return(http::Result<http::SocketHandlers>(fakeHandlers)));
    EXPECT_CALL(*pListener, startListening()).WillOnce(Return(http::Result<void>()));

    auto res = manager->startListening(handler, TEST_PORT);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(manager->getSocketClientsMap().find(15) != manager->getSocketClientsMap().end());
    EXPECT_EQ(manager->getSocketClientsMap().at(15).m_AllConnections.size(), 0);
}

//Queue methoden
TEST_F(NetworkManagerCoreTest, ReceivedQueue_successEmptyQueue_ReturnsNoError){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    http::ThreadSaveQueue<http::Request> receivedQ;

    EXPECT_CALL(*pListener, getReceivedQueue()).WillOnce(Return(&receivedQ));

    auto res = manager->try_PoPReceivedMessageQueue(handler);

    ASSERT_TRUE(res.isOK());
    EXPECT_FALSE(res.value().has_value());
}

TEST_F(NetworkManagerCoreTest, ReceivedQueue_successNotEmptyQueue_ReturnsNoError){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    http::ThreadSaveQueue<http::Request> receivedQ;
    receivedQ.push( { TEST_HCONNECTION, "Request"} );

    EXPECT_CALL(*pListener, getReceivedQueue()).WillOnce(Return(&receivedQ));

    auto res = manager->try_PoPReceivedMessageQueue(handler);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value().m_Connection, TEST_HCONNECTION);
    EXPECT_STREQ(res.value().value().m_Message.c_str(), "Request");
    EXPECT_EQ(receivedQ.size(), 0);
}

TEST_F(NetworkManagerCoreTest, ReceivedQueue_invalidLIstener_ReturnsError){
    auto res = manager->try_PoPReceivedMessageQueue(TEST_HINVALIDLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, OutgoingQueue_Success_NOError){
    HListener listener = setupListeningState(TEST_PORT, pListener);
    HSteamNetConnection con = addConnection(TEST_HSOCK);
    http::ThreadSaveQueue<http::Request> outgoingQ;
    http::Request request(con, "Request");

    EXPECT_CALL(*pListener, getOutgoingQueue()).WillOnce(Return(&outgoingQ));

    auto res = manager->push_OutgoingMessageQueue(listener, request);

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(outgoingQ.size(), 1);
}

TEST_F(NetworkManagerCoreTest, OutgoingQueue_InvalidListener_ReturnsError){
    http::Request request(TEST_HCONNECTION, "Request");
    
    auto res = manager->push_OutgoingMessageQueue(123, request);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, OutgoingQueue_ConnectionNotPresent_ReturnsError){
    HListener listener = setupListeningState(TEST_PORT, pListener);
    http::ThreadSaveQueue<http::Request> outgoingQ;
    http::Request request(TEST_HCONNECTION, "Request");

    EXPECT_CALL(*pListener, getOutgoingQueue()).Times(0);

    auto res = manager->push_OutgoingMessageQueue(listener, request);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidConnection);
}

TEST_F(NetworkManagerCoreTest, OutgoingQueue_NOtListening_ReturnsError){
    setupListener(pListener);
    HListener handler = manager->createListener("Test"); 
    http::Request request(TEST_HCONNECTION, "Request");

    EXPECT_CALL(*pListener, getOutgoingQueue()).Times(0);

    auto res = manager->push_OutgoingMessageQueue(handler, request);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerCoreTest, ErrorQueue_successEmptyQueue_RetursNoError){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>> errorQ;

    EXPECT_CALL(*pListener, getErrorQueue()).WillOnce(Return(&errorQ));

    auto res = manager->try_PoPErrorQueue(handler);

    ASSERT_TRUE(res.isOK());
    EXPECT_FALSE(res.value().has_value());
}

TEST_F(NetworkManagerCoreTest, ErrorQueue_succesNotEmptyQueue_ReturnsNoError){
    HListener handler = setupListeningState(TEST_PORT, pListener);
    http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>> errorQ;
    errorQ.push(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "TEst"));

    EXPECT_CALL(*pListener, getErrorQueue()).WillOnce(Return(&errorQ));

    auto res = manager->try_PoPErrorQueue(handler);

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(res.value().has_value());
    EXPECT_EQ(errorQ.size(), 0);
    EXPECT_EQ(res.value().value().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerCoreTest, ErrorQueue_invalidListener_ReturnsError){
    auto res = manager->try_PoPErrorQueue(123);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

//ConnectionServed
TEST_F(NetworkManagerCoreTest, ConnectionServed_Success_SetsTrue){
    HListener handler = setupListeningState(TEST_PORT, pListener);

    HSteamNetConnection con = addConnection(TEST_HSOCK);

    manager->ConnectionServed(TEST_HSOCK, con);

    auto& allClients = manager->getSocketClientsMap().at(TEST_HSOCK).m_AllConnections;
    auto it = std::find_if(allClients.begin(), allClients.end(), [con](const http::Connections& conns){
        return conns.m_connection == con;
    });

    EXPECT_TRUE(it->isServed);
}

TEST_F(NetworkManagerCoreTest, ConnectionServed_DoubleServed_showsNoReaction){
    HListener handler = setupListeningState(TEST_PORT, pListener);

    HSteamNetConnection con = addConnection(TEST_HSOCK);

    manager->ConnectionServed(TEST_HSOCK, con);

    auto& allClients = manager->getSocketClientsMap().at(TEST_HSOCK).m_AllConnections;
    auto it = std::find_if(allClients.begin(), allClients.end(), [con](const http::Connections& conns){
        return conns.m_connection == con;
    });

    EXPECT_TRUE(it->isServed);
}

//PollConnectionChanges
TEST_F(NetworkManagerCoreTest, PollConnectionChanges_succeedes_getCalled){
    EXPECT_CALL(*pMockSteam, RunCallbacks()).Times(1);

    manager->pollConnectionChanges();
}

//PollFunctionCalls
TEST_F(NetworkManagerCoreTest, pollFunctionCalls_EmptyQueue_ShowsNoReaction){
    http::ThreadSaveQueue<std::function<void()>> functionCall;

    EXPECT_NO_THROW(manager->pollFunctionCalls(&functionCall));
    EXPECT_TRUE(functionCall.empty());
}

TEST_F(NetworkManagerCoreTest, pollFunctionCalls_overMessageLimit_stops){
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

TEST_F(NetworkManagerCoreTest, pollFunctionCalls_Success_resurnsnoError){
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

