#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/Request.h"
#include "http/listener.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "steam/isteamnetworkingutils.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <cstring>
#include <functional>
#include <gtest/gtest.h>
#include <memory>
#include <sys/types.h>

using ::testing::_;
using ::testing::Return;


class ListenerCoreTest : public ::testing::Test {
protected:
    http::ListenerCore* listenerCore;
    std::shared_ptr<MOCKSteamNetworkingSockets> mockSteam;
    MOCKSteamNetworkingSockets* pMockSteam;
    std::function<void(HSteamListenSocket, HSteamNetConnection)> funcptr;
    int counter = 0;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);
        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();
        funcptr = [this](HSteamListenSocket sock, HSteamNetConnection con){
            counter++;
        };

        listenerCore = new http::ListenerCore(mockSteam, funcptr);

    }

    void TearDown() override {
        delete listenerCore;
        DESTROY_LOGGER();
    }

    void setupInitSocket( HSteamListenSocket sock, HSteamNetPollGroup pollGroup ){
        EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(sock));
        EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(pollGroup));

        auto res = listenerCore->initSocket(8080);
    }
};

//initSocket
TEST_F(ListenerCoreTest, initSocketSuccess){

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(55));

    auto res = listenerCore->initSocket(8080);

    EXPECT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_PollGroup, 55);
    EXPECT_EQ(res.value().m_Socket, 1);
}

TEST_F(ListenerCoreTest, initSocket_invalidSocketReturned){
    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).Times(0);

    auto res = listenerCore->initSocket(8080);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eSocketInitializationFailed);
}

TEST_F(ListenerCoreTest, initSocket_invalidPollGroupReturned){

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(k_HSteamNetPollGroup_Invalid));

    auto res = listenerCore->initSocket(8080);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::ePollingGroupInitializationFailed);
}

TEST_F(ListenerCoreTest, initSocket_doubleCall){
    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(55));

    auto res = listenerCore->initSocket(8080);

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).Times(0);
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).Times(0);

    auto res2 = listenerCore->initSocket(8090);

    ASSERT_TRUE(res2.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}
//DestroySocket
TEST_F(ListenerCoreTest, DestroySocket_success){
    auto* incQ = listenerCore->getReceivedQueue();
    http::Request req(222, "TEst");
    incQ->push(std::move(req));

    HSteamListenSocket sock = 1;
    HSteamNetPollGroup pollGroup = 55;
    setupInitSocket(sock, pollGroup);

    EXPECT_CALL(*pMockSteam, CloseListenSocket(sock));
    EXPECT_CALL(*pMockSteam, DestroyPollGroup(pollGroup));

    listenerCore->DestroySocket();

    EXPECT_TRUE(listenerCore->getReceivedQueue()->empty());
    EXPECT_TRUE(listenerCore->getOutgoingQueue()->empty());
    EXPECT_TRUE(listenerCore->getErrorQueue()->empty());

    setupInitSocket(70, 20);
}

TEST_F(ListenerCoreTest, DestroySocket_calledBevorInit){
    EXPECT_CALL(*pMockSteam, CloseListenSocket(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pMockSteam, DestroyPollGroup(k_HSteamNetPollGroup_Invalid));

    listenerCore->DestroySocket();
}

//pollOnce
TEST_F(ListenerCoreTest, pollOnce_onlyIncMessage){
    setupInitSocket(1, 55);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();

    const char* msg = "Test";
    SteamNetworkingMessage_t* IncMessage = SteamNetworkingUtils()->AllocateMessage(strlen(msg));
    memcpy(IncMessage->m_pData, msg, strlen(msg));
    IncMessage->m_conn = 222;

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(55, _, 1)).WillOnce([&](HSteamNetPollGroup, SteamNetworkingMessage_t** message, int){
        *message = IncMessage;
        return 1;
    });

    //pollOutMessages
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(_, _, _, _, _)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(res.value());
    EXPECT_EQ(counter, 0);
    EXPECT_TRUE(outQ->empty());
    EXPECT_EQ(inQ->size(), 1);
    auto req_or = inQ->try_pop();
    ASSERT_TRUE(req_or.has_value());
    EXPECT_EQ(req_or.value().m_Connection, 222);
    EXPECT_STREQ(req_or.value().m_Message.c_str(), "Test");
}

TEST_F(ListenerCoreTest, pollOnce_onlyOutMessage){
    setupInitSocket(1, 55);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(55, _, 1)).WillOnce(Return(0));

    //pollOutMessage
    http::Request req(222, "Request");
    const char* msg_c = req.m_Message.c_str();
    outQ->push(std::move(req));
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(222, &msg_c, (u_int32_t)strlen(msg_c), k_nSteamNetworkingSend_Reliable, nullptr)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(res.value());
    EXPECT_EQ(counter, 0);
    EXPECT_TRUE(outQ->empty());
    EXPECT_TRUE(inQ->empty());
}

TEST_F(ListenerCoreTest, pollOnce_bothPolled){
    setupInitSocket(1, 55);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();

    const char* msg = "Test";
    SteamNetworkingMessage_t* IncMessage = SteamNetworkingUtils()->AllocateMessage(strlen(msg));
    memcpy(IncMessage->m_pData, msg, strlen(msg));
    IncMessage->m_conn = 222;

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(55, _, 1)).WillOnce([&](HSteamNetPollGroup, SteamNetworkingMessage_t** message, int){
        *message = IncMessage;
        return 1;
    });

    //pollOutMessage
    http::Request req(222, "Request");
    const char* msg_c = req.m_Message.c_str();
    outQ->push(std::move(req));
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(222, &msg_c, (u_int32_t)strlen(msg_c), k_nSteamNetworkingSend_Reliable, nullptr)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(res.value());
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(outQ->empty());
    EXPECT_EQ(inQ->size(), 1);
}

TEST_F(ListenerCoreTest, pollOnce_bothDidnPoll){
    setupInitSocket(1, 55);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(55, _, 1)).WillOnce(Return(0));

    //pollOutMessages
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(_, _, _, _, _)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_FALSE(res.value());
    EXPECT_EQ(counter, 0);
    EXPECT_TRUE(outQ->empty());
    EXPECT_TRUE(inQ->empty());
}

TEST_F(ListenerCoreTest, pollOnce_ErrorOccured){
    setupInitSocket(1, 55);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();
    auto* errQ = listenerCore->getErrorQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(55, _, 1)).WillOnce(Return(-1));

    //pollOutMessages
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(_, _, _, _, _)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::ePollGroupHandlerInvalid);
    EXPECT_EQ(counter, 0);
    EXPECT_TRUE(outQ->empty());
    EXPECT_TRUE(inQ->empty());
    EXPECT_EQ(errQ->size(), 1);
}
