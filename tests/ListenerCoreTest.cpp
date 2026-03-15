#include "Logger/Logger.h"
#include "http/listener.h"
#include "mocks/SteamNetworkingSocketsAdapterMock.h"
#include "steam/steamnetworkingtypes.h"
#include <functional>
#include <gtest/gtest.h>
#include <memory>

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
