#include "gmock/gmock.h"
#include <chrono>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>

#include "Error/Errorcodes.h"
#include "http/HTTPinitialization.h"
#include "http/listener.h"
#include "mocks/MOCKListenerCore.h"
#include "steam/steamnetworkingtypes.h"

using ::testing::Return;

class ListenerTest : public ::testing::Test {
protected:
    http::Listener* listener;
    std::unique_ptr<MOCKListenerCore> Core;
    MOCKListenerCore* pCore;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);
        Core = std::make_unique<MOCKListenerCore>();
        pCore = Core.get();

        listener = new http::Listener(std::move(Core));

        ON_CALL(*pCore, pollOnce()).WillByDefault(Return(true));
    }
    void TearDown() override {
        delete listener;
        DESTROY_LOGGER();
    } 
};

//initSocket
TEST_F(ListenerTest, initSocket_success){
    http::SocketHandlers socket {1, 55};
    EXPECT_CALL(*pCore, initSocket(8080)).WillOnce(Return(socket));

    auto res = listener->initSocket(8080);

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_Socket, 1);
    EXPECT_EQ(res.value().m_PollGroup, 55);
}

TEST_F(ListenerTest, initSocket_fails){
    EXPECT_CALL(*pCore, initSocket(8080)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidCall, "invalidCall")));

    auto res = listener->initSocket(8080);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(ListenerTest, initSocketCalledwhileListening){
    http::SocketHandlers socket {1, 55};
    EXPECT_CALL(*pCore, initSocket(8080)).WillOnce(Return(socket));

    auto res = listener->initSocket(8080);
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(1));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(55));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res2 = listener->startListening();


    http::SocketHandlers socket2 {2, 56};
    EXPECT_CALL(*pCore, initSocket(8080)).Times(0);
    auto res3 = listener->initSocket(8090);

    ASSERT_TRUE(res3.isErr());
    EXPECT_EQ(res3.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

//startListening
TEST_F(ListenerTest, startListeningSuccess){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(1));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(55));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res = listener->startListening();

    ASSERT_TRUE(res.isOK());
}

TEST_F(ListenerTest, startListening_noInitSocketCall){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(k_HSteamListenSocket_Invalid));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(k_HSteamNetPollGroup_Invalid));

    EXPECT_CALL(*pCore, pollOnce()).Times(0);

    auto res = listener->startListening();

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}
//stopListening
TEST_F(ListenerTest, stopListening_whileListening){
    EXPECT_CALL(*pCore, getSocketHandler()).WillOnce(Return(1));
    EXPECT_CALL(*pCore, getPollGroup()).WillOnce(Return(55));

    EXPECT_CALL(*pCore, pollOnce()).WillRepeatedly(Return(true));

    auto res = listener->startListening();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));

    listener->stopListening();
    EXPECT_CALL(*pCore, pollOnce()).Times(0);
}

TEST_F(ListenerTest, stopListening_withoutListening){
    EXPECT_CALL(*pCore, pollOnce()).Times(0);
    EXPECT_NO_THROW(listener->stopListening());
}

//testen wir sowas wie error returned listend nicht merh?
