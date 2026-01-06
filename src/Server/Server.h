#pragma once

#include <steam/steamnetworkingsockets.h>
#include <queue>


//Problem: Zu dumm unerfahren mit framework für sockets programmieren könnten aber mit anderem step weiter machen unter annahme, dass socket alle anfragen halt immer
//in eine queue packen die dann von einem parser ausgelesen werden und interpretiert werden. Problem hab auch keine ahnung von http wie mna das parsed
//und ist halt auhc udmm auf annahmen zu arbeiten wie ist das mit threads oder kommt das direkt im socket

//Plan überlegenb entweder anderes framework oder das jertz benutzen anderes framework ist besser definitiv


namespace http{

    //steam library initen und so
    extern bool initHTTP();

    //blockende methode die nach nutzereingaben guckt und die dann an den server als commands weitergibt einfach in die message queue gepackt iwie so
    extern void listenForCommands();

    class Server {
    public:
        //init aller module aufrufen
        bool init();
        //soll auf anderem thrad laufen ig also soll nicht blockieren der server
        void run();

    public:
        Server();
        Server(const Server& other) = default;
        Server(Server&& other) = default;
        ~Server();

    private:
        bool quit = false;
        std::queue<const char*> messageQueue;

    };

}