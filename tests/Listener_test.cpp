#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>

#include "http/listener.h"
#include "mocks/MOCKListenerCore.h"

using ::testing::Return;
using ::testing::_;

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

//testen wir sowas wie error returned listend nicht merh?
//oder sowas wie das polled nicht gecalled wird wenn nicht listend??
//und das sachen in de queues abgearbeitet werdn?--> wie machen on call überschreiben?
//zudem was ist mit init socket nur wenn kein socket da ist? und sowas

