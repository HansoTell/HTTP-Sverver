
#include "NetworkManager.h"

namespace http{

void NetworkManager::init(){
    SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged( OnConnectionStatusChangedCallback );

    m_pInterface = SteamNetworkingSockets(); 
}

void NetworkManager::callbackManager( SteamNetConnectionStatusChangedCallback_t *pInfo ){


}

void NetworkManager::pollConnectionChanges(){

    while( m_Connections_open ){
        m_pInterface->RunCallbacks();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        //checken ob es noch connections gibt
        std::unique_lock<std::mutex> lock(m_connection_lock);
        if( false )
            m_Connections_open = false;

    }
}

}