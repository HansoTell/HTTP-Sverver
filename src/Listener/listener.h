#pragma once

#include <steam/steamnetworkingsockets.h>
#include <stdint.h>

namespace http{

class Listener {
public:
    Listener();
    Listener(const Listener& other) = delete;
    Listener(Listener&& other) = default;
    ~Listener();
public:
    //Soll alle clients verwalten und so und anfragen akzeptieren messages an die messager queue übergeben
    //Wie bekommen wir message queue??
    void startListening( uint16 port );
    void stopListening();
private:
    bool init( uint16 port );

    void listen();
private:
    ISteamNetworkingSockets* m_Interface = nullptr;
    HSteamListenSocket m_Socket;
};

}

