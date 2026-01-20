#include "listener.h"


//Verwaltet der Server oder der Listener seinen listen trhead auf dem gelauscht wird? Eigentlich würde listener ja mehr sinn ergeben das er sich selbst verwaltet
//aber dann müsste nochmal seperate init funktion aufgerufen werden. aber wäre ja kein ding im listen run methode am anfang listener.init aufrufen und dann hat sich der braten
//haben dan direkt thread und alles gut
//Oder sogar im konstruktor und destruktor minimiert das dauerhafrte callen noch besser

//init methode umbennenen total irreführend
namespace http{

    Listener::Listener(){
        m_running = true;
        //Listen thread starten und alles
    }


    Listener::~Listener(){
        m_running = false;
        //tghread beenden und alles
    }

    void Listener::startListening( uint16 port ){

    }

    //müssen sicher sein das port gesetzt
    Result<void> Listener::init(){

        SteamNetworkingIPAddr address;
        address.Clear();
        address.m_port = m_port;

        //Set initial Options
        int numOptions = 2;
        SteamNetworkingConfigValue_t options[numOptions];
        options[0].SetInt32(k_ESteamNetworkingConfig_TimeoutInitial, 5000);
        options[1].SetInt32(k_ESteamNetworkingConfig_TimeoutConnected, 5000);


        m_Socket = NetworkManager::Get().m_pInterface->CreateListenSocketIP(address, numOptions, options);

        if( m_Socket == k_HSteamListenSocket_Invalid){
            auto error = MAKE_ERROR(HTTPErrors::eSocketInitializationFailed, "Failed to initialize Socket");             
            LOG_VERROR(error, "On Port", m_port);
            return error; 
        }

        NetworkManager::Get().startCallbacksIfNeeded();

        m_pollGroup = NetworkManager::Get().m_pInterface->CreatePollGroup();

        if( m_pollGroup == k_HSteamNetPollGroup_Invalid ){
            auto error = MAKE_ERROR(HTTPErrors::ePollingGroupInitializationFailed, "Failed to initialize Polling Group");
            LOG_VERROR(error, "On Port" , m_port);
            return error; 
        }

        //funktioiert das mit dem void das dann returend wird? wahrscheinlich momentan nicht ig aber einfach anpassungen
        return;
    }

    void Listener::listen(){
        //muss noch nicht der finale thread methode sein geht nur darum,
        //2 While schleifen in einander: Eine läuft solange initialisiert erst im destruktor beenden (-> was wenn wartend und dann notyfied?)
        //Dann init von 
        //müssen die variablen überhaupt atomic sein wer ändet die
        //brauchen externe Methoden die dann das start ubnd stop listening bedingen: Also eine Methode die Server aufrfen kann ja setzt listening true oder halt nicht
        std::unique_lock<std::mutex> _lock (m_ListenMutex); 
        while ( m_running )
        {
            m_ListenCV.wait(_lock, [this](){
                return !m_running || m_listening;
            });

            _lock.unlock();

            if( !m_running ) 
                break;

            //Eher soaws wie init_Socket oder so ka was weiß ich
            //Poert muss iwie rein kommen und das fehl schlagen kann natürlich ein großes Problem
            //Sogar ganz großes problem weil wie bekomme ich die nachricht raus aus dem Thread ich mein loggen ok aber was machen wir sonst iwiwe muss er ja die nachricht bekommen
            init();
            while( m_listening ){

                //poll messegas und son scheiß u know
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            //Eher sowas wie destroy Socket oder sowas oder End Connections iwie sowas
            stopListening();

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