#include "listener.h"


namespace http{

    Listener::Listener(){

    }

    Listener::~Listener(){

    }

    bool Listener::init( uint16 port ){
        m_Interface = SteamNetworkingSockets();

        SteamNetworkingIPAddr address;
        address.Clear();
        address.m_port = port;

        //Dokumentation lesen und optinen setzten
        SteamNetworkingConfigValue_t opt;

        m_Socket = m_Interface->CreateListenSocketIP(address, 1, &opt);


        //pollgtroup dokumentation lesen allgemeijn alles an dokumentation so schenll lesen wie geht
    }

    void Listener::listen(){

    }
}