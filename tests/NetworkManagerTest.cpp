#include "gmock/gmock.h"
#include <http/NetworkManager.h>

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

        mockSteam = std::make_unique<MOCKSteamNetworkingSockets>();
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

        manager->init(std::move(core), mockSteam);
    }

    //da fehlen einige sachen was mit init oder kill double calls. Und retunrn wir errors?
}; 
