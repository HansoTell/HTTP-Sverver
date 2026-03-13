#include "gmock/gmock.h"
#include <http/NetworkManager.h>

#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "mocks/MOCKNetworkManagerCore.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "gtest/gtest.h"
#include <memory>
#include <utility>

using ::testing::Return;
using ::testing::_;

class NetworkManagerTest : public ::testing::Test {
protected:
    http::NetworkManager* manager;
    std::unique_ptr<MOCKNetworkManagerCore> core;
    MOCKNetworkManagerCore* pCore;
    std::shared_ptr<MOCKSteamNetworkingSockets> mockSteam;
    MOCKSteamNetworkingSockets* pMockSteam;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();

        core = std::make_unique<MOCKNetworkManagerCore>();
        pCore = core.get();

        manager = &(http::NetworkManager::Get());

    }

    void TearDown() override {
        manager->kill();
        DESTROY_LOGGER();
    }

    void initManager() {
        EXPECT_CALL(*pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).WillOnce(Return(true));

        auto res = manager->init(std::move(core), mockSteam);

        EXPECT_TRUE(res.isOK());
    }
}; 

//init
TEST_F(NetworkManagerTest, init_Success){
        EXPECT_CALL(*pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).WillOnce(Return(true));

        auto res = manager->init(std::move(core), mockSteam);

        EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, init_DoubleCall){
    initManager();

    std::shared_ptr<MOCKSteamNetworkingSockets> double_mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
    MOCKSteamNetworkingSockets* double_pMockSteam = double_mockSteam.get();
    std::unique_ptr<MOCKNetworkManagerCore> doulble_core = std::make_unique<MOCKNetworkManagerCore>();
    MOCKNetworkManagerCore* double_pCore = core.get();

    auto res2 = manager->init(std::move(doulble_core), double_mockSteam);

    EXPECT_CALL(*double_pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).Times(0); 
    ASSERT_TRUE(res2.isErr());
    EXPECT_EQ(res2.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

//Kill
TEST_F(NetworkManagerTest, kill_withoutInit){
    EXPECT_NO_THROW(manager->kill());
}

TEST_F(NetworkManagerTest, kill_success){
    initManager();

    EXPECT_NO_THROW(manager->kill());


    //wie teste ich ob der thread noch läuft?
    //keine ahnung vielleicht test methode dazu keine ahnung
}

//FunctionCallMethods



