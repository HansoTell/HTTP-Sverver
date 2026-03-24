#include "gmock/gmock.h"
#include <chrono>
#include <future>
#include <http/NetworkManager.h>

#include "Error/Errorcodes.h"
#include "Logger/Logger.h"
#include "http/HTTPinitialization.h"
#include "http/Request.h"
#include "mocks/Test_Constants.h"
#include "mocks/MOCKNetworkManagerCore.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "gtest/gtest.h"
#include <memory>
#include <optional>
#include <thread>
#include <utility>
#include <vector>

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

TEST_F(NetworkManagerTest, init_CallbackPtrFails){
    EXPECT_CALL(*pMockSteam, SetGlobalCallback_SteamNetConnectionStatusChanged(_)).WillOnce(Return(false));

    auto res = manager->init(std::move(core), mockSteam);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInternalError);
    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_FALSE(manager->isThreadJoinable());
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

    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_FALSE(manager->isThreadJoinable());
}

TEST_F(NetworkManagerTest, kill_calledWhileActiveCalls_DoesntDeadlock){
    initManager();

    EXPECT_CALL(*pCore, createListener("Test")).WillOnce(Return(1));
    auto res = manager->createListener("Test");

    manager->kill();

    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_FALSE(manager->isThreadJoinable());
}

TEST_F(NetworkManagerTest, kill_whileConnectionsActive_DoesntBlock){
    EXPECT_CALL(*pCore, isSocketClientsMapEmpty()).WillRepeatedly(Return(false));
    initManager();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    manager->kill();

    EXPECT_FALSE(manager->isInitialized());
    EXPECT_FALSE(manager->isRunning());
    EXPECT_FALSE(manager->isThreadJoinable());
}

//FunctionCallMethods
TEST_F(NetworkManagerTest, createListener_success){
    initManager();

    EXPECT_CALL(*pCore, createListener("Test")).WillOnce(Return(1));

    auto res = manager->createListener("Test");

    ASSERT_TRUE(res.isOK());
    EXPECT_EQ(res.value(), 1);
}

TEST_F(NetworkManagerTest, createListener_withoutInit_ReturnsError){
    EXPECT_CALL(*pCore, createListener("Test")).Times(0);

    auto res = manager->createListener("Test");

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, DestroyListener_invalidListener){
    initManager();
    EXPECT_CALL(*pCore, DestroyListener(_)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->DestroyListener(TEST_HLISTENER);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, DestroyListener_calledWithoutInit_ReturnError){
    EXPECT_CALL(*pCore, DestroyListener(_)).Times(0);

    auto res = manager->DestroyListener(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, DestroyListener_success_ReturnsNoError){
    initManager();
    EXPECT_CALL(*pCore, DestroyListener(_)).WillOnce(Return(http::Result<void>()));

    auto res = manager->DestroyListener(TEST_HLISTENER);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, startListening_success){
    initManager();

    EXPECT_CALL(*pCore, startListening(TEST_HLISTENER, TEST_PORT)).WillOnce(Return(http::Result<void>()));

    auto res = manager->startListening(TEST_HLISTENER, TEST_PORT);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, startListening_invalidListener){
    initManager();

    EXPECT_CALL(*pCore, startListening(TEST_HLISTENER, TEST_PORT)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->startListening(TEST_HLISTENER, TEST_PORT);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, startListening_socketInitFailed){
    initManager();

    EXPECT_CALL(*pCore, startListening(TEST_HLISTENER, TEST_PORT)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eSocketInitializationFailed, "bsp")));

    auto res = manager->startListening(TEST_HLISTENER, TEST_PORT);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eSocketInitializationFailed);
}

TEST_F(NetworkManagerTest, startListening_noInitCalled_ReturnsError){

    EXPECT_CALL(*pCore, startListening(TEST_HLISTENER, TEST_PORT)).Times(0);

    auto res = manager->startListening(TEST_HLISTENER, TEST_PORT);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, stopListening_success){
    initManager();

    EXPECT_CALL(*pCore, stopListening(TEST_HLISTENER)).WillOnce(Return(http::Result<void>()));

    auto res = manager->stopListening(TEST_HLISTENER);

    EXPECT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, stopListening_InvalidListener){
    initManager();

    EXPECT_CALL(*pCore, stopListening(TEST_HLISTENER)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "bsp")));

    auto res = manager->stopListening(TEST_HLISTENER);

    EXPECT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, stopListening_CalledWithoutInit_ReturnsError){
    EXPECT_CALL(*pCore, stopListening(TEST_HLISTENER)).Times(0);

    auto res = manager->stopListening(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}


TEST_F(NetworkManagerTest, ConnectionServed_Succes){
    initManager();

    std::promise<void> prms;

    EXPECT_CALL(*pCore, ConnectionServed(TEST_PORT, TEST_HCONNECTION)).WillOnce([&](){
        prms.set_value();
    });

    auto res = manager->ConnectionServed(TEST_PORT, TEST_HCONNECTION);

    ASSERT_EQ(prms.get_future().wait_for(std::chrono::milliseconds(100)), std::future_status::ready);

    ASSERT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, ConnectionServed_CalledWithNoInit_ReturnsError){
    EXPECT_CALL(*pCore, ConnectionServed(TEST_PORT, TEST_HCONNECTION)).Times(0);

    auto res = manager->ConnectionServed(TEST_PORT, TEST_HCONNECTION);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
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
    std::promise<void> prms;

    EXPECT_CALL(*pCore, ConnectionServed(TEST_PORT, TEST_HCONNECTION)).WillOnce([&](){
        prms.set_value();
    });

    http::NetworkManager::sConnectionServedCallback(TEST_PORT, TEST_HCONNECTION);

    ASSERT_EQ(prms.get_future().wait_for(std::chrono::milliseconds(100)), std::future_status::ready);
}

TEST_F(NetworkManagerTest, popReceivedQueue_sccess){
    initManager();

    EXPECT_CALL(*pCore, try_PoPReceivedMessageQueue(TEST_HLISTENER)).WillOnce(Return( std::optional<http::Request>({ TEST_HCONNECTION, "Test" }) ));

    auto res = manager->try_PoPReceivedMessageQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value().m_Connection, TEST_HCONNECTION);
}

TEST_F(NetworkManagerTest, popReceivedQueue_calledWithourInit_ReturnsError){
    EXPECT_CALL(*pCore, try_PoPReceivedMessageQueue(TEST_HLISTENER)).Times(0);

    auto res = manager->try_PoPReceivedMessageQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, popReceivedQueue_invalidListener){
    initManager();

    EXPECT_CALL(*pCore, try_PoPReceivedMessageQueue(TEST_HLISTENER)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "invalidListener")));

    auto res = manager->try_PoPReceivedMessageQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, pushOutQ_success){
    initManager();
    http::Request req(TEST_HCONNECTION, "Req");

    EXPECT_CALL(*pCore, push_OutgoingMessageQueue(TEST_HLISTENER, req)).WillOnce(Return( http::Result<void>() ));

    auto res = manager->push_OutgoingMessageQueue(TEST_HLISTENER, req);

    ASSERT_TRUE(res.isOK());
}

TEST_F(NetworkManagerTest, pushOutQ_WithoutInit_ReturnsError){
    http::Request req(TEST_HCONNECTION, "Req");

    EXPECT_CALL(*pCore, push_OutgoingMessageQueue(TEST_HLISTENER, req)).Times(0);

    auto res = manager->push_OutgoingMessageQueue(TEST_HLISTENER, req);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, pushOutQ_invalidListener){
    initManager();
    http::Request req(TEST_HCONNECTION, "Req");

    EXPECT_CALL(*pCore, push_OutgoingMessageQueue(TEST_HLISTENER, req)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "invalidListener")));

    auto res = manager->push_OutgoingMessageQueue(TEST_HLISTENER, req);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, pushOutQ_invalidCall){
    initManager();
    http::Request req(TEST_HCONNECTION, "Req");

    EXPECT_CALL(*pCore, push_OutgoingMessageQueue(TEST_HLISTENER, req)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidCall, "invalidCall")));

    auto res = manager->push_OutgoingMessageQueue(TEST_HLISTENER, req);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, popErrorQ_success){
    initManager();

    EXPECT_CALL(*pCore, try_PoPErrorQueue(TEST_HLISTENER)).WillOnce(Return( std::optional<Error::ErrorValue<http::HTTPErrors>>( MAKE_ERROR(http::HTTPErrors::eInvalidCall, "bsp")) ));

    auto res = manager->try_PoPErrorQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isOK());
    ASSERT_TRUE(res.value().has_value());
    EXPECT_EQ(res.value().value().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, popErrorQ_WithoutInitCalled_ReturnsError){
    EXPECT_CALL(*pCore, try_PoPErrorQueue(TEST_HLISTENER)).Times(0);

    auto res = manager->try_PoPErrorQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidCall);
}

TEST_F(NetworkManagerTest, popErrorQ_invalidListener){
    initManager();

    EXPECT_CALL(*pCore, try_PoPErrorQueue(TEST_HLISTENER)).WillOnce(Return(MAKE_ERROR(http::HTTPErrors::eInvalidListener, "invalidListener")));

    auto res = manager->try_PoPErrorQueue(TEST_HLISTENER);

    ASSERT_TRUE(res.isErr());
    EXPECT_EQ(res.error().ErrorCode, http::HTTPErrors::eInvalidListener);
}

TEST_F(NetworkManagerTest, executeFunctionBlocksTillDone){
    initManager();

    std::promise<void> started;
    std::promise<void> released;

    EXPECT_CALL(*pCore, createListener(_)).WillOnce([&](auto){
        started.set_value();
        released.get_future().wait();
        return 33;
    });

    std::thread t ([&](){
        auto res = manager->createListener("Test");
        ASSERT_TRUE(res.isOK());
        EXPECT_EQ(res.value(), 33);
    });

    ASSERT_EQ(started.get_future().wait_for(std::chrono::milliseconds(100)), std::future_status::ready);

    released.set_value();

    t.join();
}

TEST_F(NetworkManagerTest, executeFunction_preservesOrder){
    initManager();

    std::vector<int> order;

    EXPECT_CALL(*pCore, createListener(_)).Times(3).WillRepeatedly([&](){
        order.push_back(order.size());

        return 1;
    });

    for(int i = 0; i < 3; i++){
        manager->createListener("Test");
    }

    EXPECT_EQ(order, std::vector<int>({0,1,2}));
}

TEST_F(NetworkManagerTest, executeFunction_threadSave){
    initManager();
    std::vector<std::thread> threads;

    EXPECT_CALL(*pCore, createListener(_)).Times(10).WillRepeatedly(Return(TEST_HLISTENER));

    for(int i = 0; i < 10; i++){
        threads.emplace_back([&](){
            manager->createListener("Test");
        });
    }

    for(auto& t : threads) t.join();
}
