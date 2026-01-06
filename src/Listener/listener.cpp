#include "listener.h"


namespace http{

    Listener::Listener(){

    }

    Listener::~Listener(){

    }

    void Listener::startListening( uint16 port ){
        //sollte nicht doppelt init gecalled werden wenn stop listenin und wieder start listening oder?? 
        //zudem kann init fehlschlgane? dann fehljer output
        init( port );
        listen();
    }

    bool Listener::init( uint16 port ){

        if( !m_Interface )
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
        //natürlich soll nicht für immer wheil true sein, soll auch stoppbar sein
        while (true)
        {

        }
        
    }

    void Listener::stopListening(){

        //alles wichtige erst noch handeln oder alle connections einzeln schließen nur demonstration

        

        m_Interface->CloseListenSocket( m_Socket );
    }
}