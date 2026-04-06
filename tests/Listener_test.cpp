#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>

#include "Error/Errorcodes.h"
#include "http/listener.h"
#include "mocks/MOCKListenerCore.h"
#include "steam/steamnetworkingtypes.h"
#include "mocks/Test_Constants.h"

using ::testing::Return;

class ListenerTest : public ::testing::Test {
protected:
    std::unique_ptr<http::Listener> listener;
    std::unique_ptr<MOCKListenerCore> Core;
    MOCKListenerCore* pCore;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);
        Core = std::make_unique<MOCKListenerCore>();
        pCore = Core.get();

        listener = std::make_unique<http::Listener>(std::move(Core));

        ON_CALL(*pCore, pollOnce()).WillByDefault(Return(true));
    }
    void TearDown() override {
        listener.reset();
        DESTROY_LOGGER();
    } 
};

//initSocket
TEST_F(ListenerTest, initSocket_success_returnNoError){
    http::SocketHandlers socket { TEST_HSOCK, TEST_HPOLLGROUP};
    EXPECT_CALL(*pCore, initSocket(TEST_PORT)).WillOnce(Return(socket));

    auto res = listener->initSocket(TEST_PORT);

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_Socket, TEST_HSOCK);
    EXPECT_EQ(res.value().m_PollGroup, TEST_HPOLLGROUP);
}

TEST_F(ListenerTest, initSocket_fails_ReturnsInvalidCallError){
    EXPECT_CALL(*pCore, initSocket(TEST_PORT)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidCall, "invalidCall")));

    auto res = listener->initSocket(TEST_PORT);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(ListenerTest, initSocket_CalledwhileListening_ReturnsInvalidCallError){
    http::SocketHandlers socket {TEST_HSOCK, TEST_HPOLLGROUP};
    EXPECT_CALL(*pCore, initSocket(TEST_PORT)).WillOnce(Return(socket));

    auto res = listener->initSocket(TEST_PORT);
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res2 = listener->startListening();


    http::SocketHandlers socket2 {2, 56};
    EXPECT_CALL(*pCore, initSocket(TEST_PORT)).Times(0);
    auto res3 = listener->initSocket(8090);

    ASSERT_TRUE(res3.isErr());
    EXPECT_EQ(res3.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

//startListening
TEST_F(ListenerTest, startListening_Success_returnNoError){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res = listener->startListening();

    ASSERT_TRUE(res.isOK());
    EXPECT_TRUE(listener->isListening());
}

TEST_F(ListenerTest, startListening_InvalidSocketHandler_ReturnsInvalidCallError){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pCore, getPollGroup()).Times(0);

    EXPECT_CALL(*pCore, pollOnce()).Times(0);

    auto res = listener->startListening();

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
    EXPECT_FALSE(listener->isListening());
}

TEST_F(ListenerTest, startListening_PollGroupInvalid_ReturnInvalidCallError){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(k_HSteamNetPollGroup_Invalid));

    EXPECT_CALL(*pCore, pollOnce()).Times(0);

    auto res = listener->startListening();

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
    EXPECT_FALSE(listener->isListening());
}
//stopListening
TEST_F(ListenerTest, stopListening_whileListening_succeedes){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res = listener->startListening();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    EXPECT_CALL(*pCore, DestroySocket()).Times(1);

    listener->stopListening();
    EXPECT_CALL(*pCore, pollOnce()).Times(0);
    EXPECT_FALSE(listener->isListening());
}

TEST_F(ListenerTest, stopListening_withoutListening_showsNoReaction){
    EXPECT_CALL(*pCore, pollOnce()).Times(0);
    EXPECT_NO_THROW(listener->stopListening());
    EXPECT_FALSE(listener->isListening());
}

TEST_F(ListenerTest, start_listening_pollOnceReturnError_stopsListening){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(TEST_HSOCK));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(TEST_HPOLLGROUP));

    EXPECT_CALL(*pCore, pollOnce())
        .WillOnce(Return(true))
        .WillOnce(Return(MAKE_ERROR(http::HTTPErrors::ePollGroupHandlerInvalid, "Test")));

    auto res = listener->startListening();
    ASSERT_TRUE(res.isOK());

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_CALL(*pCore, pollOnce()).Times(0);
    EXPECT_FALSE(listener->isListening());
}

