#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/Request.h"
#include "http/listener.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "steam/steamclientpublic.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <sys/poll.h>
#include <sys/types.h>

#define TEST_HSOCK 1
#define TEST_HPOLLGROUP 55
#define TEST_HCONNECTION 222
#define TEST_PORT 8080

using ::testing::_;
using ::testing::Return;


class ListenerCoreTest : public ::testing::Test {
protected:
    std::unique_ptr<http::ListenerCore> listenerCore;
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

        listenerCore = std::make_unique<http::ListenerCore>(mockSteam, funcptr);
    }

    void TearDown() override {
        DESTROY_LOGGER();
    }

    void setupInitSocket( HSteamListenSocket sock, HSteamNetPollGroup pollGroup ){
        EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(sock));
        EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(pollGroup));

        auto res = listenerCore->initSocket(TEST_PORT);

        ASSERT_TRUE(res.isOK());
    }
};


//initSocket
TEST_F(ListenerCoreTest, initSocketSuccess){

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    auto res = listenerCore->initSocket(TEST_PORT);

    EXPECT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_PollGroup, TEST_HPOLLGROUP);
    EXPECT_EQ(res.value().m_Socket, TEST_HSOCK);
    EXPECT_EQ(listenerCore->getSocketHandler(), TEST_HSOCK);
    EXPECT_EQ(listenerCore->getPollGroup(), TEST_HPOLLGROUP);
}

TEST_F(ListenerCoreTest, initSocket_invalidSocketReturned){
    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).Times(0);

    auto res = listenerCore->initSocket(TEST_PORT);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eSocketInitializationFailed);
    EXPECT_EQ(listenerCore->getSocketHandler(), k_HSteamListenSocket_Invalid);
    EXPECT_EQ(listenerCore->getPollGroup(), k_HSteamNetPollGroup_Invalid);
}

TEST_F(ListenerCoreTest, initSocket_invalidPollGroupReturned){

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(k_HSteamNetPollGroup_Invalid));
    EXPECT_CALL(*pMockSteam, CloseListenSocket(TEST_HSOCK)).Times(1);

    auto res = listenerCore->initSocket(TEST_PORT);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::ePollingGroupInitializationFailed);
    EXPECT_EQ(listenerCore->getSocketHandler(), k_HSteamListenSocket_Invalid);
    EXPECT_EQ(listenerCore->getPollGroup(), k_HSteamNetPollGroup_Invalid);
}

TEST_F(ListenerCoreTest, initSocket_doubleCall){
    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    auto res = listenerCore->initSocket(TEST_PORT);

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).Times(0);
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).Times(0);

    auto res2 = listenerCore->initSocket(8090);

    ASSERT_TRUE(res2.isErr());
    EXPECT_EQ(res2.error().ErrorCode, http::HTTPErrors::eInvalidCall);
    EXPECT_EQ(listenerCore->getSocketHandler(), TEST_HSOCK);
    EXPECT_EQ(listenerCore->getPollGroup(), TEST_HPOLLGROUP);
}
//DestroySocket
TEST_F(ListenerCoreTest, DestroySocket_success){
    auto* incQ = listenerCore->getReceivedQueue();
    http::Request req(TEST_HCONNECTION, "TEst");
    incQ->push(std::move(req));

    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);

    EXPECT_CALL(*pMockSteam, CloseListenSocket(TEST_HSOCK));
    EXPECT_CALL(*pMockSteam, DestroyPollGroup(TEST_HPOLLGROUP));

    listenerCore->DestroySocket();

    EXPECT_TRUE(listenerCore->getReceivedQueue()->empty());
    EXPECT_TRUE(listenerCore->getOutgoingQueue()->empty());
    EXPECT_TRUE(listenerCore->getErrorQueue()->empty());
}

TEST_F(ListenerCoreTest, DestroySocket_calledBevorInit){
    EXPECT_CALL(*pMockSteam, CloseListenSocket(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pMockSteam, DestroyPollGroup(k_HSteamNetPollGroup_Invalid));

    listenerCore->DestroySocket();
}

TEST_F(ListenerCoreTest, DestroySocket_initDestroyInit){
    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);

    EXPECT_CALL(*pMockSteam, CloseListenSocket(TEST_HSOCK));
    EXPECT_CALL(*pMockSteam, DestroyPollGroup(TEST_HPOLLGROUP));

    listenerCore->DestroySocket();

    HSteamListenSocket sock2 = 2;
    HSteamNetPollGroup poll2 = 56;

    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(sock2));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(poll2));

    auto res = listenerCore->initSocket(8080);
    EXPECT_EQ(res.value().m_Socket, sock2);

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_Socket, 2);
    EXPECT_EQ(res.value().m_PollGroup, 56);
    EXPECT_EQ(listenerCore->getSocketHandler(), 2);
    EXPECT_EQ(listenerCore->getPollGroup(), 56);
}

//pollOnce
struct pollTestCase {
    bool incoming;
    bool outgoing;
    bool expectedResult;
};
class PollOnceTest : public ListenerCoreTest, public ::testing::WithParamInterface<pollTestCase> {};

INSTANTIATE_TEST_SUITE_P(PollTests, PollOnceTest, ::testing::Values(
    pollTestCase{false, false, false},
    pollTestCase{true, false, true},
    pollTestCase{false, true, true},
    pollTestCase{true, true, true}
));

TEST_P(PollOnceTest, PollBehaviour){
    auto param = GetParam();

    int expectedCounter = 0;
    int expectedOutQSize = 0;
    int expectedInQSize = 0;

    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();


    //pollIncMessage
    if( param.incoming )
    {
        auto* IncMessage = new FakeSteamNetowrkingMessage("Test", TEST_HCONNECTION);

        EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(TEST_HPOLLGROUP, _, 1)).WillOnce(testing::DoAll(
            testing::SetArgPointee<1>(IncMessage), 
            Return(1)
        ));

        expectedInQSize = 1;
    } else 
    {
        EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(TEST_HPOLLGROUP, _, 1)).WillOnce(Return(0));
    }

    //pollOutMessage
    if( param.outgoing )
    {
        http::Request req(TEST_HCONNECTION, "Request");
        outQ->push(std::move(req));
        EXPECT_CALL(*pMockSteam, SendMessageToConnection(TEST_HCONNECTION, _, 7, k_nSteamNetworkingSend_Reliable, nullptr)).WillOnce(Return(k_EResultOK));
        expectedCounter = 1;
    }

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(res.value(), param.expectedResult);
    EXPECT_EQ(counter, expectedCounter);
    EXPECT_EQ(outQ->size(), expectedOutQSize);
    EXPECT_EQ(inQ->size(), expectedInQSize);

    if( param.incoming )
    {
        auto req_or = inQ->try_pop();
        ASSERT_TRUE(req_or.has_value());
        EXPECT_EQ(req_or.value().m_Connection, TEST_HCONNECTION);
        EXPECT_STREQ(req_or.value().m_Message.c_str(), "Test");
    }
}

TEST_F(ListenerCoreTest, pollOnce_OutMessageEmptyString){
    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(TEST_HPOLLGROUP, _, 1)).WillOnce(Return(0));

    //pollOutMessage
    http::Request req(TEST_HCONNECTION, "");
    const char* msg_c = req.m_Message.c_str();
    outQ->push(std::move(req));
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(TEST_HCONNECTION, _, _, k_nSteamNetworkingSend_Reliable, nullptr)).Times(0);

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(res.value());
    EXPECT_EQ(counter, 0);
    EXPECT_TRUE(outQ->empty());
    EXPECT_TRUE(inQ->empty());
}

TEST_F(ListenerCoreTest, pollOnce_ErrorOccured){
    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();
    auto* errQ = listenerCore->getErrorQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(TEST_HPOLLGROUP, _, 1)).WillOnce(Return(-1));

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

TEST_F(ListenerCoreTest, pollOnce_PollIncPollErr){
    setupInitSocket(TEST_HSOCK, TEST_HPOLLGROUP);
    auto* outQ = listenerCore->getOutgoingQueue();
    auto* inQ = listenerCore->getReceivedQueue();
    auto* errQ = listenerCore->getErrorQueue();

    //pollIncMessage
    EXPECT_CALL(*pMockSteam, ReceiveMessagesOnPollGroup(TEST_HPOLLGROUP, _, 1)).WillOnce(Return(-1));

    //pollOutMessage
    http::Request req(TEST_HCONNECTION, "Request");
    const char* msg_c = req.m_Message.c_str();
    outQ->push(std::move(req));
    EXPECT_CALL(*pMockSteam, SendMessageToConnection(TEST_HCONNECTION, _, 7, k_nSteamNetworkingSend_Reliable, nullptr)).WillOnce(Return(k_EResultOK));

    auto res = listenerCore->pollOnce();

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::ePollGroupHandlerInvalid);
    EXPECT_EQ(counter, 1);
    EXPECT_TRUE(outQ->empty());
    EXPECT_TRUE(inQ->empty());
    EXPECT_EQ(errQ->size(), 1);
}
