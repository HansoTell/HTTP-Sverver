#pragma once

#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <sys/types.h>
#include "http/NetworkManager.h"
#include "steam/steamnetworkingtypes.h"

class MOCKNetworkManagerCore : public http::INetworkManagerCore {
public:
    MOCK_METHOD(HListener, createListener, (const char*), (override));
    MOCK_METHOD(http::Result<void>, DestroyListener, (HListener), (override));
    MOCK_METHOD(http::Result<void>, startListening, (HListener, u_int16_t), (override));
    MOCK_METHOD(http::Result<void>, stopListening, (HListener), (override));
    MOCK_METHOD(void, ConnectionServed, (HSteamListenSocket, HSteamNetConnection), (override));
    MOCK_METHOD(void, pollConnectionChanges, (), (override));
    MOCK_METHOD(void, pollFunctionCalls, (http::ThreadSaveQueue<std::function<void()>>*), (override));
    MOCK_METHOD(void, callbackManager, (SteamNetConnectionStatusChangedCallback_t*), (override));
    MOCK_METHOD(bool, isSocketClientsMapEmpty, (), (override));
    MOCK_METHOD(http::Result<std::optional<http::Request>>, try_PoPReceivedMessageQueue, (HListener), (override));
    MOCK_METHOD(http::Result<void>, push_OutgoingMessageQueue, (HListener, http::Request), (override));
    MOCK_METHOD(http::Result<std::optional<Error::ErrorValue<http::HTTPErrors>>>, try_PoPErrorQueue, (HListener), (override));
};
