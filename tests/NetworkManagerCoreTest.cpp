#include "Error/Errorcodes.h"
#include "http/listener.h"
#include "steam/isteamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"
#include "gmock/gmock.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <sys/types.h>


class MOCKSteamNetworkingSockets : public ISteamNetworkingSockets { 
public:
    MOCK_METHOD(EResult, AcceptConnection, (HSteamNetConnection), (override));
    MOCK_METHOD(bool, CloseConnection, ( HSteamNetConnection, int, const char *, bool), (override));
    MOCK_METHOD(bool, SetConnectionPollGroup, ( HSteamNetConnection, HSteamNetPollGroup ), (override));
    MOCK_METHOD(void, RunCallbacks, (), (override));
};

class MOCKListener : public http::Listener{
public:
    MOCK_METHOD(http::Result<http::SocketHandlers>, initSocket, (u_int16_t), (override));
    MOCK_METHOD(void, startListening, (), (override));
    MOCK_METHOD(void, stopListening, (), (override));

    MOCK_METHOD(http::ThreadSaveQueue<Error::ErrorValue<http::HTTPErrors>>*, getErrorQueue, (), (override) );
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getReceivedQueue, (), (override));
    MOCK_METHOD(http::ThreadSaveQueue<http::Request>*, getOutgoingQueue, (), (override));
};

class MOCKListenerFactory : public http::IListenerFactory {
    MOCK_METHOD(std::unique_ptr<http::IListener>, createListener, (), (override));
};
