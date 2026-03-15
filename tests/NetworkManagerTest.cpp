#include "gmock/gmock.h"
#include <chrono>
#include <future>
#include <http/NetworkManager.h>

#include "Datastrucutres/ThreadSaveQueue.h"
#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "mocks/MOCKNetworkManagerCore.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "gtest/gtest.h"
#include <memory>
#include <optional>
#include <thread>
#include <utility>

using ::testing::Return;
using ::testing::_;

class NetworkManagerTest : public ::testing::Test {
protected:
    http::NetworkManager* manager;
    std::unique_ptr< testing::NiceMock<MOCKNetworkManagerCore>> core;
    testing::NiceMock<MOCKNetworkManagerCore>* pCore;
    std::shared_ptr<MOCKSteamNetworkingSockets> mockSteam;
    MOCKSteamNetworkingSockets* pMockSteam;

    void SetUp() override {
        CREATE_LOGGER("NetworkmanagerCoreTests.log");
        SET_LOG_LEVEL(Log::LogLevel::ERROR);

        mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
        pMockSteam = mockSteam.get();

        core = std::make_unique<testing::NiceMock<MOCKNetworkManagerCore>>();
        pCore = core.get();

        manager = &(http::NetworkManager::Get());

        ON_CALL(*pCore, pollFunctionCalls(_)).WillByDefault([](auto queue){
            u_int16_t counter = 0;
            while(!queue->empty() && counter < 20){
                auto funk_opt = queue->try_pop();

                if( !funk_opt.has_value() )
                    break;
                auto funk = funk_opt.value();
                funk();

                counter++;
            }
        });

        ON_CALL(*pCore, pollConnectionChanges()).WillByDefault(Return());

        ON_CALL(*pCore, isSocketClientsMapEmpty()).WillByDefault(Return(true));
    }

    void TearDown() override {
        manager->kill();
        DESTROY_LOGGER();
    }

    void initManager() {
        EXPECT_CALL(*pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).WillOnce(Return(true));

        auto res = manager->init(std::move(core), mockSteam);

        ASSERT_TRUE(res.isOK());
    }
}; 

//init
TEST_F(NetworkManagerTest, init_Success){
    EXPECT_CALL(*pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).WillOnce(Return(true));

    auto res = manager->init(std::move(core), mockSteam);

    EXPECT_TRUE(res.isOK());
    EXPECT_TRUE(manager->isInitialized());
    EXPECT_TRUE(manager->isRunning());
    EXPECT_TRUE(manager->isThreadJoinable());
}

TEST_F(NetworkManagerTest, init_DoubleCall){
    initManager();

    std::shared_ptr<MOCKSteamNetworkingSockets> double_mockSteam = std::make_shared<MOCKSteamNetworkingSockets>();
    MOCKSteamNetworkingSockets* double_pMockSteam = double_mockSteam.get();
    std::unique_ptr<MOCKNetworkManagerCore> doulble_core = std::make_unique<MOCKNetworkManagerCore>();
    MOCKNetworkManagerCore* double_pCore = doulble_core.get();

    EXPECT_CALL(*double_pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).Times(0); 

    auto res2 = manager->init(std::move(doulble_core), double_mockSteam);

    ASSERT_TRUE(res2.isErr());
    EXPECT_EQ(res2.error().ErrorCode, http::HTTPErrors::eInvalidCall);
    EXPECT_TRUE(manager->isInitialized());
    EXPECT_TRUE(manager->isRunning());
    EXPECT_TRUE(manager->isThreadJoinable());
}

//Kill
TEST_F(NetworkManagerTest, kill_withoutInit){
    EXPECT_NO_THROW(manager->kill());
}

TEST_F(NetworkManagerTest, kill_success){
    initManager();

    EXPECT_NO_THROW(manager->kill());

    //halt nicht testet ob core nullptr ist 
    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_FALSE(manager->isThreadJoinable());
}

//FunctionCallMethods
TEST_F(NetworkManagerTest, createListener_success){
    initManager();

    EXPECT_CALL(*pCore, createListener("Test")).WillOnce(Return(1));

    HListener handler = manager->createListener("Test");

    EXPECT_EQ(handler, 1);
}
TEST_F(NetworkManagerTest, DestroyListener_invalidListener){
    initManager();
    EXPECT_CALL(*pCore, DestroyListener(_)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->DestroyListener(12345);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, DestroyListener_success){
    initManager();
    EXPECT_CALL(*pCore, DestroyListener(_)).WillOnce(Return(http::Result<void>()));

    auto res = manager->DestroyListener(12345);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, startListening_success){
    initManager();

    EXPECT_CALL(*pCore, startListening(12345, 8080)).WillOnce(Return(http::Result<void>()));

    auto res = manager->startListening(12345, 8080);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, startListening_invalidListener){
    initManager();

    EXPECT_CALL(*pCore, startListening(12345, 8080)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->startListening(12345, 8080);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, startListening_socketInitFailed){
    initManager();

    EXPECT_CALL(*pCore, startListening(12345, 8080)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eSocketInitializationFailed, "bsp")));

    auto res = manager->startListening(12345, 8080);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eSocketInitializationFailed);
}

TEST_F(NetworkManagerTest, stopListening_success){
    initManager();

    EXPECT_CALL(*pCore, stopListening(12345)).WillOnce(Return(http::Result<void>()));

    auto res = manager->stopListening(12345);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, stopListening_InvalidListener){
    initManager();

    EXPECT_CALL(*pCore, stopListening(12345)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->stopListening(12345);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}


TEST_F(NetworkManagerTest, ConnectionServed_Succes){
    initManager();

    std::promise<void> prmis;
    auto future = prmis.get_future();
    EXPECT_CALL(*pCore, ConnectionServed(8080, 222)).Times(1);

    manager->ConnectionServed(8080, 222);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

TEST_F(NetworkManagerTest, RunCallbacks){
    initManager();
    SteamNetConnectionStatusChangedCallback_t Info{}; 
    EXPECT_CALL(*pCore, callbackManager(&Info)).Times(1);

    manager->runCallbacks(&Info);
}

TEST_F(NetworkManagerTest, staticRunCallbacks){
    initManager();
    SteamNetConnectionStatusChangedCallback_t Info{}; 
    EXPECT_CALL(*pCore, callbackManager(&Info)).Times(1);

    http::NetworkManager::sOnConnectionStatusChangedCallback(&Info);
}

TEST_F(NetworkManagerTest, staticConnectionServed){
    initManager();

    EXPECT_CALL(*pCore, ConnectionServed(8080, 222)).Times(1);
    http::NetworkManager::sConnectionServedCallback(8080, 222);

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

TEST_F(NetworkManagerTest, popReceivedQueue_sccess){
    initManager();

    EXPECT_CALL(*pCore, try_PoPReceivedMessageQueue(12345)).WillOnce(Return( std::optional<http::Request>({ 222, "Test" }) ));

    auto res = manager->try_PoPReceivedMessageQueue(12345);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value().m_Connection, 222);
}

//getQueueMethoden fehlen
//fehlet mhrfache belastung und vielleicht reihenfolge oder so
