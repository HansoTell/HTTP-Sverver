
#include "Logger/Logger.h"
#include "mocks/MOCKNetworkManagerCore.h"
#include "gtest/gtest.h"
#include <http/NetworkManager.h>
#include <memory>
#include <utility>


class NetworkManagerTest : public ::testing::Test {
protected:
    http::NetworkManager* manager;
    std::unique_ptr<MOCKNetworkManagerCore> core;
    MOCKNetworkManagerCore* pCore;
    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        core = std::make_unique<MOCKNetworkManagerCore>();
        pCore = core.get();

        manager = &(http::NetworkManager::Get());
        
        manager->init(std::move(core));

    }

    void TearDown() override {
        manager->kill();
        DESTROY_LOGGER();
    }

}; 



