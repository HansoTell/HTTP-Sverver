#include "HTTPinitialization.h"

#include "../Listener/NetworkManager.h"
#include <filesystem>

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

    g_Logger = std::make_unique<Log::Logger>(logDir/"app.log");
    g_Logger->setLogLevel( LOGLEVEL );

    //iwie so message am besten auch noch zurück geben oder so
    char a[1024]; 
    if( !GameNetworkingSockets_Init(nullptr, a) ){

        return false;
    }

    NetworkManager::Get().init();

    isHTTPInitialized = true;

    return true;
}

bool HTTP_Kill(){
    g_Logger.reset(nullptr);

    //Alles runter fahren falls wir noch mehr brauchen
    GameNetworkingSockets_Kill();
    NetworkManager::Get().kill();

    isHTTPInitialized = false;

    return true;
}

void listenForCommands(){

}


}
