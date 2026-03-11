#include <http/HTTPinitialization.h>

#include "Logger/Logger.h"
#include "http/NetworkManager.h"
#include <filesystem>
#include <memory>

namespace http{

bool isHTTPInitialized = false;

bool initHTTP(){

    std::filesystem::path logDir;

    if( const char* env = std::getenv("APP_LOG_DIR") ){
        logDir = env;
    }else{
        logDir = "Logs";
    }

    //was wenn schon existiert?
    std::filesystem::create_directory(logDir);

    CREATE_LOGGER(logDir/"app.log");

    //iwie so message am besten auch noch zurück geben oder so
    char a[1024]; 
    if( !GameNetworkingSockets_Init(nullptr, a) ){

        return false;
    }

    NetworkManager::Get().init( std::make_shared<SteamNetworkingSocketsAdapter>( SteamNetworkingSockets() ) );

    isHTTPInitialized = true;

    return true;
}

bool HTTP_Kill(){
    DESTROY_LOGGER();

    //Alles runter fahren falls wir noch mehr brauchen
    GameNetworkingSockets_Kill();
    NetworkManager::Get().kill();

    isHTTPInitialized = false;

    return true;
}

void listenForCommands(){

}


}
