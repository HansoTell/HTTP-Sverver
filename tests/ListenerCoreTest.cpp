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

TEST_F(ListenerCoreTest, initSocketSuccess){

    //eingabe fixen
    EXPECT_CALL(*pMockSteam, CreateListenSocketIP(_, _, _)).WillOnce(Return(1));
    EXPECT_CALL(*pMockSteam, CreatePollGroup()).WillOnce(Return(55));

    auto res = listenerCore->initSocket(8080);

    //testen wir ob internal die sockets gesetzt sind??
    EXPECT_TRUE(res.isOK());
    EXPECT_EQ(res.value().m_PollGroup, 55);
    EXPECT_EQ(res.value().m_Socket, 8080);

}
