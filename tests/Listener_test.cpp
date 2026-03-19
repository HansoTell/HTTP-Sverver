#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "http/listener.h"
#include "mocks/MOCKListenerCore.h"


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
    }
    void TearDown() override {
        delete listener;
        DESTROY_LOGGER();
    } 
};

