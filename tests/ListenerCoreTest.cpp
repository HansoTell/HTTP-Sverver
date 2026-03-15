#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/listener.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <functional>
#include <gtest/gtest.h>
#include <memory>

using ::testing::_;
using ::testing::Return;

class ListenerCoreTest : public ::testing::Test {
protected:
    http::ListenerCore* listenerCore;
    std::shared_ptr<MOCKSteamNetworkingSockets> mockSteam;
    MOCKSteamNetworkingSockets* pMockSteam;
    std::function<void(HSteamListenSocket, HSteamNetConnection)> funcptr;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);
        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();
        //mal gucken
        funcptr = [](HSteamListenSocket sock, HSteamNetConnection con){

        };

        listenerCore = new http::ListenerCore(mockSteam, funcptr);

    }

    void TearDown() override {
        delete listenerCore;
        DESTROY_LOGGER();
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


//DestroySocket
