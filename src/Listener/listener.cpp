#include "listener.h"


namespace http{

    Listener::Listener(){

    }



    Listener::~Listener(){

    }

    void Listener::startListening( uint16 port ){
        //init kann fehlschlagen also false returnen im fehlschlagen oder so oder error objekt alt (expected clon)
        if( !init( port )){
            //error handeling
            //und so weoter wahrschienlich false retunrn oder iein error objekt oder sowas ka
        }
        listen();
    }

    bool Listener::init( uint16 port ){

        SteamNetworkingIPAddr address;
        address.Clear();
        address.m_port = port;

        //Set initial Options
        int numOptions = 2;
        SteamNetworkingConfigValue_t options[numOptions];
        options[0].SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, 5000);
        options[1].SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, 5000);


        m_Socket = NetworkManager::Get().m_pInterface->CreateListenSocketIP(address, numOptions, options);

        if( m_Socket == k_HSteamListenSocket_Invalid){
            //error
            //loggen
            return false;
        }

        NetworkManager::Get().startCallbacksIfNeeded();

        m_pollGroup = NetworkManager::Get().m_pInterface->CreatePollGroup();

        if( m_pollGroup == k_HSteamNetPollGroup_Invalid ){
            //error
            //loggen
            return false;
        }
    }

    void Listener::listen(){
        while (true)
        {

        }
        
    }

    void Listener::stopListening(){

        //alles wichtige erst noch handeln oder alle connections einzeln schließen nur demonstration


        //muss ich das socket auf 0 setzten oder so? damit dann nicht falsche dinge passieren
        //muss ich poll group auf null setzten oder so und mus ich checkenb ob false returned wird
        NetworkManager::Get().m_pInterface->DestroyPollGroup( m_pollGroup );
        NetworkManager::Get().notifySocketDestruction( m_Socket );
        NetworkManager::Get().m_pInterface->CloseListenSocket( m_Socket );
    }
}