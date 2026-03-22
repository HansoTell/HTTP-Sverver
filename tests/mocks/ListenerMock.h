#pragma once
#include "gmock/gmock.h"
#include <gmock/gmock.h>

#include "http/HTTPinitialization.h"
#include "http/listener.h"

class MOCKListener : public http::IListener{
public:
    MOCK_METHOD(http::Result<http::SocketHandlers>, initSocket, (u_int16_t), (override));
    MOCK_METHOD(http::Result<void>, startListening, (), (override));
    MOCK_METHOD(void, stopListening, (), (override));

    MOCK_METHOD(http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>>*, getErrorQueue, (), (override) );
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getReceivedQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getOutgoingQueue, (), (override));
    MOCK_METHOD(bool, isListening, (), (override));
};

class MOCKListenerFactory : public http::IListenerFactory {
public:
    MOCK_METHOD(std::unique_ptr<http::IListener>, createListener, (), (override));
};
