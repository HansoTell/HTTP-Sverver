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

    bool init( uint16 port );

    //Soll alle clients verwalten und so und anfragen akzeptieren messages an die messager queue übergeben
    //Wie bekommen wir message queue??
    void listen();

private:
    ISteamNetworkingSockets* m_Interface;
    HSteamListenSocket m_Socket;
};

}

