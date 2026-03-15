#pragma once

#include "http/listener.h"
#include "gmock/gmock.h"
#include <gmock/gmock.h>
#include <sys/types.h>

class MOCKListenerCore : public http::IListenerCore {
public:
    MOCK_METHOD(http::Result<http::SocketHandlers>, initSocket, (u_int16_t), (override));
    MOCK_METHOD(void, DestroySocket, (), (override));
    MOCK_METHOD(http::Result<bool>, pollOnce, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getReceivedQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getOutgoingQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>>*, getErrorQueue, (), (override));
};



