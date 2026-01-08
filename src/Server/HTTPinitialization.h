#pragma once


#include <steam/steamnetworkingsockets.h>


#include "NetworkManager.h"

namespace http{

extern bool isHTTPInitialized;

//steam library initen und so
extern bool initHTTP();

//was erschaffen wird muss auch getötet werden
extern bool HTTP_Kill();

//blockende methode die nach nutzereingaben guckt und die dann an den server als commands weitergibt einfach in die message queue gepackt iwie so
extern void listenForCommands();


}