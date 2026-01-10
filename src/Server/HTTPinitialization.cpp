#include "HTTPinitialization.h"

namespace http{

//file einfügen wo ich das will
Log::Logger g_Logger("");

bool isHTTPInitialized = false;

bool initHTTP(){
    //iwie so message am besten auch noch zurück geben oder so
    char a[1024]; 
    if( !GameNetworkingSockets_Init(nullptr, a) ){

        return false;
    }

    isHTTPInitialized = true;

    NetworkManager::Get().init();
}

bool HTTP_Kill(){
    //Alles runter fahren falls wir noch mehr brauchen
    GameNetworkingSockets_Kill();
    NetworkManager::Get().kill();

    isHTTPInitialized = false;
}

void listenForCommands(){

}


}