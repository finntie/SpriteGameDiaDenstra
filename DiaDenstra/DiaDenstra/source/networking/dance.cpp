#include "networking/dance.h"

// Networking classes
#include <iphlpapi.h>  //For retrieving private IP
#include <windows.h>
#include <ws2tcpip.h>

#include <stdio.h>
#include <sstream>
#include <iostream>
#include <map>
#include <chrono>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Iphlpapi.lib")

//---------------------------------------------------------------------------------------
//  Code was made using help from https://beej.us/guide/bgnet/pdf/bgnet_usl_c_1.pdf
//---------------------------------------------------------------------------------------

Dance::~Dance()
{
    //If still connections, tell that we disconnected
    const char* message = "Disconnect";
    for (const auto& pair : them_addrss)
    {
        sendMessage(message, int(strlen(message)), pair.second, false, nullptr);
    }

    // Close socket so that recvfrom() will end
    shutdown(sockfd, SD_BOTH);
    closesocket(sockfd);
    // Wait for thread
    if (connectionMade.valid())
    {
        quitListening = true;
        quitCallBack = true;
        connectionMade.get();  // Waits for the thread to finish execution
    }
    if (listenThread.joinable())
    {
        quitListening = true;
        listenThread.join();
    }
    if (callBackThread.joinable())
    {
        quitCallBack = true;
        callBackThread.join();
    }
    them_numbers.clear();
    them_addrss.clear();
}

void Dance::init(bool useCallBack, bool _ForceIPV4)
{
    // First pass the version of winsock
    // Latest version is 2.2, this one we want to use.
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOGLINE(DLogObj, DanceLogger::DANCE_CRITICAL, "WSAStartup failed");
        exit(1);
    }
    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        WSACleanup();  // We are done with winsock
        LOGLINE(DLogObj, DanceLogger::DANCE_CRITICAL, "Version 2.2 of winsock is not available");
        exit(2);
    }

    DLogObj.setBreakSeverity(DanceLogger::DANCE_ERROR);
    //LOGLINE(DLogObj, DanceLogger::severityEnum::DANCE_CRITICAL, "AHHH EVERTHING WENT WRONG, CODE RED! CODE RED!");
    //LOGLINE(DLogObj, DanceLogger::severityEnum::DANCE_ERROR, "Initialization error, no credit to BUAS in code");
    //LOGLINE(DLogObj, DanceLogger::severityEnum::DANCE_WARNING, "It seems this could be done differently, how did you end up here?");
    //LOGLINE(DLogObj, DanceLogger::severityEnum::DANCE_INFO, "Nothing on the hand, just some info");
    DLogObj.printIssues(DanceLogger::DANCE_INFO, false, false);

    useCallBackFunctions = useCallBack;
    forceIPV4 = _ForceIPV4;
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//-----------------------------------      Connecting devices to eachother       --------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------


void Dance::Host(DanceMoves moves, int maxConnections, const char* port, bool _forceIPV4, const std::vector<std::string>& clientsIP)
{
    
    //--------------------------------------Set Variables--------------------------------
    

    forceIPV4 = _forceIPV4;
    if (forceIPV4) ipv = AF_INET;
    currentMoves = moves;
    isHost = true;
    maxConnections = maxConnections;


    // Check if the inserted port is valid, otherwise just use the hostPort
    int testPort = 0;
    if (port != NULL) testPort = atoi(port);
    if (testPort <= 1024 || testPort >= 49151)
    {
        port = hostPort;
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Default port: %s is used", hostPort);
        LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
    }
    else
    {
        memcpy_s(hostPort, sizeof(hostPort), port, strlen(port));
    }

    
    //-----------------------------------------Set IPs-----------------------------------
    

    char otherIP[39] = " ";
    char ownIP[39] = " ";

    // If we are on the same device, we just use 'localhost' as our IP.
    if (moves == SAMEDEVICE)
    {
        // Use strcpy_s to set otherIP to localhost
        const char* localHostInput = "localhost";
        strcpy_s(otherIP, localHostInput);
        strcpy_s(ownIP, localHostInput);
        ipv = AF_INET;
    }
    else
    {
        const bool p = moves == PUBLIC ? true : false;
        strcpy_s(ownIP, getIP(p).c_str());  // Set ownIP using getIP()
    }

    /*
    -----------------------------------------Important variables-----------------------------------
    */

    struct addrinfo hints, * res;
    int status;
    const char yes = '1';

    /*
    --------------------------------------Add client IP to connections--------------------------------
    */

    if (moves == PUBLIC)
    {
        // Put all the clients IPs in the map
        for (int i = 0; i < int(clientsIP.size()); i++)
        {
            strcpy_s(otherIP, clientsIP[i].c_str());

            memset(&hints, 0, sizeof(hints));
            hints.ai_family = ipv;  // Use IPV4 or IPV6
            hints.ai_socktype = SOCK_DGRAM;
            // addrinfo
            if ((status = getaddrinfo(otherIP, hostPort, &hints, &res)) != 0)
            {
                char errorBuffer[256];
                sprintf_s(errorBuffer, sizeof(errorBuffer), "(Invalid IP) getaddrinfo error on clients IP %s", getSendError(WSAGetLastError()));
                LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
                continue;
            }
            // Get name of ourself
            char nameInfoHost[256];
            char nameInfoServ[256];
            // TODO: if IP is wrong this can take ages. (sometimes seconds)
            getnameinfo(res->ai_addr,
                int(res->ai_addrlen),
                nameInfoHost,
                sizeof(nameInfoHost),
                nameInfoServ,
                sizeof(nameInfoServ),
                0);

            // Add host to map
            them_addrss.emplace(nameInfoHost, res);
            them_numbers.emplace(nameInfoHost, 0);
            //Don't Increase total connections (We do this after we confirmed the connection in holepunching)
            //totalConnections++;
        }
    }

    /*
    -------------------------------------------Winsock2 Code---------------------------------------
    */

    // Get Address information for this device
    res = nullptr;
    memset(&hints, 0, sizeof hints);  // Reset Hints
    hints.ai_family = ipv;            // Use IPV4 or IPV6
    hints.ai_socktype = SOCK_DGRAM;   // Use UPD

    // If we are binding using ipv4, we use our private IP
    char ownIPV2[65];
    if (moves == PUBLIC && ipv == AF_INET)
    {
        strcpy_s(ownIPV2, getIP(false).c_str());
    }
    else
    {
        strcpy_s(ownIPV2, ownIP);
    }

    // Get address info of the host (ourself)
    if ((status = getaddrinfo(ownIPV2, hostPort, &hints, &res)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "getaddrinfo error: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        return;
    }

    // Get name of ourself
    char nameInfoHost[256];
    char nameInfoServ[256];
    getnameinfo(res->ai_addr, int(res->ai_addrlen), nameInfoHost, sizeof(nameInfoHost), nameInfoServ, sizeof(nameInfoServ), 0);
    deviceName = nameInfoHost;

    // Create Socket
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == INVALID_SOCKET)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Error on socket creation: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        return;
    }

    // Loose the "socket already in use" error
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "setsockopt error: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
        return;
    }

    // Bind
    if (bind(sockfd, res->ai_addr, int(res->ai_addrlen)) == -1)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Error on binding: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        return;
    }

    freeaddrinfo(res);  // Free the data.

    /*
    ---------------------------------------Wait for Connections-------------------------------------
    */

    // If name is "NONE", use the device name or in the case of the same device, use program ID
    if (name == "NONE") name = deviceName;
    // IGNORE ALL ABOVE LETS CALL US 'HOST'
    name = "HOST";
    // This we do so we can indentify if we are the host, but maybe this will get removed later.

    // If we use a public network, we need to hole punch
    if (moves == DanceMoves::PUBLIC)
    {
        if (listenThread.joinable())
        {
            listenThread.join();
        }
        if (callBackThread.joinable())
        {
            callBackThread.join();
        }

        //Used as listenThread, since this will transform into listening after its done
        listenThread = std::thread([this] { this->holePunch(); });
        return;
    }

    //First check and possibly clean up old mess.
    if (listenThread.joinable())
    {
        listenThread.join();
    }
    if (callBackThread.joinable())
    {
        callBackThread.join();
    }

    // Now we just need to wait for incomming connections.
    // Create a thread that will listen for a return message, this will add connections to our map of 
    listenThread = std::thread([this] { this->listen(true); });
    // Another one for the call backs
    if (useCallBackFunctions) callBackThread = std::thread([this] { this->sendCallBacks(); });
}

bool Dance::Connect(DanceMoves moves, const char* hostIP, const char* port, bool _forceIPV4)
{
    
    // --------------------------------------Set Variables--------------------------------
    
    char otherIP[39] = " ";
    char ownIP[39] = " ";

    forceIPV4 = _forceIPV4;
    if (forceIPV4) ipv = AF_INET;
    currentMoves = moves;
    isHost = false;
    strcpy_s(otherIP, hostIP);

    // Check if the inserted port is valid, otherwise just use the hostPort
    int testPort = 0;
    if (port != NULL) testPort = atoi(port);
    if (testPort <= 1024 || testPort >= 49151)
    {
        port = hostPort;
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Default port: %s is used", hostPort);
        LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
    }
    else
    {
        memcpy_s(hostPort, sizeof(hostPort), port, strlen(port));
    }

    
    // -----------------------------------------Set IPs-----------------------------------
    

    // If we are on the same device, we just use 'localhost' as our IP.
    if (moves == SAMEDEVICE)
    {
        // Use strcpy_s to set otherIP to localhost
        const char* localHostInput = "localhost";
        strcpy_s(otherIP, localHostInput);
        strcpy_s(ownIP, localHostInput);
        ipv = AF_INET; //Set to IPV4
    }
    else
    {
        const bool p = moves == PUBLIC ? true : false;
        strcpy_s(ownIP, getIP(p).c_str());  // Set ownIP using getIP()
    }

    
    // -----------------------------------------Important variables-----------------------------------
    

    struct addrinfo hints, * res, * dest;
    int status;

    
    // --------------------------------------Add host IP to connections--------------------------------
    

    // Handle this and put it inside the map
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = ipv;
    hints.ai_socktype = SOCK_DGRAM;
    // addrinfo   
    if ((status = getaddrinfo(otherIP, hostPort, &hints, &res)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "getaddrinfo error on host: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        resetState();
        return false;
    }

    // Add host to map
    them_addrss.emplace("HOST", res);
    them_numbers.emplace("HOST", 0);

    
    // --------------------------------------Get Address information for this device--------------------------------
   

    res = nullptr;
    memset(&hints, 0, sizeof hints);  // Reset Hints
    hints.ai_family = ipv;            // Use IPV4 or IPV6
    hints.ai_socktype = SOCK_DGRAM;   // Use UPD
    hints.ai_protocol = IPPROTO_UDP;

    // If we are binding using ipv4, we use our private IP
    char ownIPV2[65];
    if (moves == PUBLIC && ipv == AF_INET)
    {
        strcpy_s(ownIPV2, getIP(false).c_str());
    }
    else
    {
        strcpy_s(ownIPV2, ownIP);
    }

    // Get address info of ourself
    if ((status = getaddrinfo(ownIPV2, hostPort, &hints, &res)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "getaddrinfo error: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        resetState();
        return false;
    }

    // Get name of ourself
    char nameInfoHost[256];
    char nameInfoServ[256];
    getnameinfo(res->ai_addr, int(res->ai_addrlen), nameInfoHost, sizeof(nameInfoHost), nameInfoServ, sizeof(nameInfoServ), 0);
    deviceName = nameInfoHost;

    
    // --------------------------------------Create Socket and possibly bind--------------------------------
    

    // Create Socket
    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == INVALID_SOCKET)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Error on socket creation: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        resetState();
        return false;
    }

    // Bind only if we use public IP, we dont need it elsewhere
    if (moves == PUBLIC)
    {
        if (bind(sockfd, res->ai_addr, int(res->ai_addrlen)) == -1)
        {
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "Error on binding: %s", getSendError(WSAGetLastError()));
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
            resetState();
            return false;
        }
    }

    freeaddrinfo(res);  // Free the data. (We don't need to use it anymore)

    // ---------------------------------------------Connect To Host---------------------------------------------------
    

    // If name is "NONE", use the device name or in the case of the same device, use program ID
    if (moves == SAMEDEVICE && name == "NONE")
    {
        name = std::to_string(GetCurrentProcessId());
    }
    else if (name == "NONE")
    {
        name = deviceName;
    }

    // If we use a public network, we need to hole punch
    if (moves == PUBLIC)
    {
        if (listenThread.joinable())
        {
            listenThread.join();
        }
        if (callBackThread.joinable())
        {
            callBackThread.join();
        }

        //Used as listenThread, since this will transform into listening after its done
        listenThread = std::thread([this] { this->holePunch(); });

        return true;
    }

    // Send a message to the host to connect
    // message Structure: 'Con' + 'name of the device'
    std::string connectMessage = "Con" + name;

    // Get address info of the host
    if ((status = getaddrinfo(otherIP, hostPort, &hints, &dest)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "getaddrinfo error: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        resetState();
        return false;
    }

    //Connect for windows to send the error message to this socket, otherwise it may not does this
    if (connect(sockfd, dest->ai_addr, int(dest->ai_addrlen)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "connection error: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
        resetState();
        return false;
    }

    //Using send (not sendto), because we are connected and have set up a default location
    if (send(sockfd, connectMessage.c_str(), int(connectMessage.size()), 0) == -1)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Send failed: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
        resetState();
        return false;
    }

    Sleep(50); //Wait for package to arrive safely 
    char buffer[NORMALPACKAGESIZE];
    int result = 0;

    //Unblock socket
    u_long iMode = 1;
    if (ioctlsocket(sockfd, FIONBIO, &iMode) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Could not unblock socket: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
        resetState();
        return false;
    }

    if ((result = recv(sockfd, buffer, NORMALPACKAGESIZE, MSG_PEEK)) < 0) //We can safely consume this message, since this is a conConfirm. 
    {
        if (WSAGetLastError() == 10035) // WSAEWOULDBLOCK, not fatal error, no data available YET
        {
            // Issue could be that the host is slow in processing messages, but we are able to reach the host at least.
            LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, "Errorcode: 10035, no data available from the host yet. Proceeded connection.");

            //We want to make sure it arrived, so add it to the important buffer
            addImportantMessage(connectMessage, 0);
        }
        else if (WSAGetLastError() == 10054) //Connection reset by peer.
        {
            LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, "Could not connect to socket, is the host connected?");
            resetState();
            return false;
        }
        else
        {
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "Receive error trying to connect, did the host lag out?: %s", getSendError(WSAGetLastError()));
            LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
            resetState();
            return false;
        }
    }

    //Block socket again
    iMode = 0;
    if (ioctlsocket(sockfd, FIONBIO, &iMode) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Could not block socket: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
        resetState();
        return false;
    }

    freeaddrinfo(dest);

    
    // --------------------------------------Create Threads---------------------------------------
    

    //First check and possibly clean up old mess.
    if (listenThread.joinable())
    {
        listenThread.join();
    }
    if (callBackThread.joinable())
    {
        callBackThread.join();
    }

    // Now create a thread that we will listen on indeffinitely
    listenThread = std::thread([this] { this->listen(true); });

    // Another one for the call backs
    if (useCallBackFunctions) callBackThread = std::thread([this] { this->sendCallBacks(); });

    return true;
}

void Dance::holePunch()
{
    holePunchingStatus = 1;

    // Generate seed
    const unsigned seed = int(std::chrono::system_clock::now().time_since_epoch().count());
    // Use this seed to get random numbers
    srand(seed);

    // Create vector of all connections we need to make
    std::vector<struct addrinfo*> futureConnections;

    for (const auto& it : them_addrss)
    {
        futureConnections.push_back(it.second);
    }

    // While-loop until we connected to everyone
    while (futureConnections.size() > 0)
    {
        // Thread to listen
        connectionMade = std::async(std::launch::async, [this] { return this->listen(false); });

        while (true)
        {
            const auto status = connectionMade.wait_for(std::chrono::milliseconds(100));
            if (status == std::future_status::ready)
            {
                if (connectionMade.get()) break;  // Connection was successful
                //Note that it is possible for status to be true, but we already had them, so we did not progress.
            }

            // Sending message
            const std::string holePunchingMessage = "Con " + name + ": HolePunch";

            // If we succefully connected on the first iteration, it will finish the other 9 before realizing
            for (int j = 0; j < 10; j++)
            {
                // Iterate over all the other peers
                for (int i = 0; i < int(futureConnections.size()); i++)
                {
                    sendMessage(holePunchingMessage.c_str(), int(holePunchingMessage.size()), futureConnections[i], false, nullptr);
                }
                // Sleep for between 0.09 and 0.11 seconds
                Sleep(rand() % 2 + 9);
            }
        }

        // Check which one connected and remove it from the vector
        for (int i = 0; i < int(holePunchConfirmedConnections.size()); i++)
        {
            for (int j = 0; j < int(futureConnections.size()); j++)
            {
                if (holePunchConfirmedConnections[i]->ai_addrlen == futureConnections[j]->ai_addrlen &&
                    isSameAdress((sockaddr_storage*)holePunchConfirmedConnections[i]->ai_addr, (sockaddr_storage*)futureConnections[j]->ai_addr))
                {
                    // remove from futureconnections
                    futureConnections.erase(futureConnections.begin() + j);
                }
            }
        }
    }
    //Clear vector, we dont need it anymore
    holePunchConfirmedConnections.clear();
    holePunchingStatus = 2;

    //Create a thread for callback
    if (useCallBackFunctions) callBackThread = std::thread([this] { this->sendCallBacks(); });

    //Now start listening until stopped
    listen(true);
}


void Dance::KeepAlive(float dt)
{
    if (currentMoves == PUBLIC && isHost)
    {
        keepAliveTime -= dt;
        if (keepAliveTime <= 0.0f)
        {
            keepAliveTime = 60.0f;

            const char* message = "KeepAliveMessage";
            // Send message to everyone.
            for (const auto& pair : them_addrss)
            {
                sendMessage(message, int(strlen(message)), pair.second, false, nullptr);
            }
        }
    }
    //else //Send to everyone a garbage message
    //{
    //    quickKeepAliveTime -= dt;
    //    if (quickKeepAliveTime <= 0.0f)
    //    {
    //        quickKeepAliveTime = 0.5f;

    //        const char* message = "KeepAliveMessage";
    //        // Send message to everyone.
    //        for (const auto& pair : them_addrss)
    //        {
    //            sendMessage(message, int(strlen(message)), pair.second, false, nullptr);
    //        }
    //    }
    //}
}

void Dance::getPackage(std::string PackageName, bool DeleteMessage, ReturnPackageInfo* packageInfo)
{
    messageVectorMutex.lock();

    bool isList = false;
    for (int i = int(receivedMessages.size()) - 1; i >= 0; --i)
    {
        bool gotName = false;
        std::string tmp;
        int wordIndex = 0;
        packageInfo->succeeded = true;

        while (!(tmp = getWord(receivedMessages[i], wordIndex++)).empty())
        {
            // First index is the name
            if (!gotName)
            {
                gotName = true;
                continue;  // Can use later
            }

            // Check if this is the list we are searching for
            if (!isList)
            {
                if (tmp != PackageName)
                    break;
                else
                {
                    isList = true;
                    continue;  // Go to next word
                }
            }

            if (tmp == "end") break;

            bool parsed = false;

            std::apply( //Apply unpacks the tuple arguments into the lamda so types will become the types in the tuple.
                [&](auto... types)//lambda takes a reference pack of all the tuple types.
                {
                    (... || (parsed = tryParse(tmp, types),  // Try to parse each empty type, parsed will be true or false, types will be filled with input if succeeded
                        parsed ? (packageInfo->VariableVector.push_back(types), true)
                        : false));  // If parsed, push back the type and return true, else return false.
                },
                std::tuple<int, float, double, unsigned, bool, std::string>{});  // Types that we will test

            if (!parsed)
            {
                //Just insert the string
                packageInfo->VariableVector.push_back(tmp);
                //Let know that not all variables succeeded to parse
                packageInfo->succeeded = false;
            }
        }
        //Gathered all variables (hopefully)
        if (DeleteMessage && packageInfo->succeeded && isList)
        {
            printf("Removed message %s from message vector\n", receivedMessages[i].c_str());
            receivedMessages.erase(receivedMessages.begin() + i);  // Delete this message if we want to.
        }
        if (isList) break;
    }
    if (!isList) packageInfo->succeeded = false;
    messageVectorMutex.unlock();
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//-----------------------------------      Listen for incomming messages       ----------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------




bool Dance::listen(bool keepChecking)
{
    const int MaxMessagesVectorSize = 100;
    struct sockaddr_storage them_addr {};  // Adress of the others

    // Up the writing size and reading size
    int sendBufferSize = 2 * 1024;   // 2 KB send buffer size
    int recvBufferSize = 32 * 1024;  // 32 KB receive buffer size
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char*)&sendBufferSize, sizeof(sendBufferSize)) < 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Error setting send buffer size: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
    }
    // Set receive buffer size
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char*)&recvBufferSize, sizeof(recvBufferSize)) < 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Error setting receive buffer size: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);
    }

    //Unblock
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);

    //Set current time based on time since program started
    lastTime = static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());

    // Use timeout using poll
    WSAPOLLFD pollfd;
    pollfd.fd = sockfd;
    pollfd.events = POLLIN;

    quitListening = false;
    while (!quitListening)
    {
        const int gotMessage = WSAPoll(&pollfd, 1, 50); // 50 ms timeout
        if (gotMessage == 0)
        {
            //Not in yet, so in the meantime lets check the important messages
            checkOnImportantMessages();
            continue;
        }
        else if (gotMessage < 0)
        {
            //Error
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "Error receiving message: %s", getSendError(WSAGetLastError()));
            LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
            continue;
        }
        else if (gotMessage > 0 && (pollfd.revents & POLLIN))
        {
            //We got a message!

            memset(listenBuffer, 0, MAXPACKAGESIZE * sizeof(char));
            socklen_t addrSize = sizeof(sockaddr_storage);
            const int bufferLen = recvfrom(sockfd, (char*)listenBuffer, sizeof(listenBuffer), 0, (struct sockaddr*)&them_addr, &addrSize);
            if (bufferLen < 0)
            {
                // Highly likely that the connection was reset by peer, if not, check it out!
                const bool removedPeer = handleDisconnection(them_addr, addrSize);

                if (removedPeer && !isHost) //TODO: what if Public connection
                {
                    quitListening = true;
                    listenThread.detach();
                    return false;
                }
                if (!removedPeer)
                {
                    char errorBuffer[256];
                    sprintf_s(errorBuffer, sizeof(errorBuffer), "Error receiving message: %s", getSendError(WSAGetLastError()));
                    LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
                }
            }
            else if (bufferLen == 0)
            {
                char errorBuffer[256];
                sprintf_s(errorBuffer, sizeof(errorBuffer), "Buffer of 0 received, connection closed?: %s", getSendError(WSAGetLastError()));
                LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
            }
            else
            {
                // Check buffer size DEBUG
                // u_long bytesAvailable;
                // if (ioctlsocket(sockfd, FIONREAD, &bytesAvailable) == 0)
                //{
                //     std::cout << "Bytes available in the receive buffer: " << bytesAvailable << std::endl;
                // }
                // else
                //{
                //     perror("Error querying receive buffer size\n");
                // }

                if (bufferLen >= MAXPACKAGESIZE)
                {
                    char errorBuffer[256];
                    sprintf_s(errorBuffer, sizeof(errorBuffer), "Message too long: %s", getSendError(WSAGetLastError()));
                    LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
                    continue;
                }

                //Received message
                printf("Message received: %s\n", listenBuffer);

                bool ConMessage = false;
                std::string otherName{};
                listenBuffer[bufferLen] = '\0';  // Null terminate the received message

                //---------------------------------------------------Important Message?-------------------------------------------

                //Check if it is an important message
                if (std::strncmp(listenBuffer, "Imp", 3) == 0)
                {
                    int messResult = listenImportantMessage(them_addr, addrSize);
                    if (messResult == 1)
                    {
                        continue;
                    }
                }

                //Check if it is a succes important message
                if (std::strncmp(listenBuffer, "SImp", 4) == 0)
                {
                    int messResult = listenSuccesImportantMessage();
                    if (messResult == 1)
                    {
                        continue;
                    }
                }

                //Let's check on the other important messages
                checkOnImportantMessages();


                //---------------------------------------------------Connection Message?-------------------------------------------

                // Check if this is a connection message
                if (std::strncmp(listenBuffer, "Con", 3) == 0)
                {
                    // If it is a ConConfirm, and we don't want to keep checking, we can return
                    if (std::strncmp(listenBuffer, "ConConfirm", 10) == 0)
                    {
                        // Set our number
                        sscanf_s(&listenBuffer[10], "%d", &peer_ID);
                        if (keepChecking)
                        {
                            continue;
                        }
                        else
                        {
                            return true;
                        }
                    }
                    // Check if it is above our max connections
                    if (them_addrss.size() >= maxConnections) continue;
                    ConMessage = true;
                    otherName = &listenBuffer[3];  // Name starts after the 'Con'
                }

                //Check if it is a disconnect message
                else if (std::strncmp(listenBuffer, "Dis", 3) == 0)
                {
                    if (handleDisconnection(them_addr, addrSize))
                    {
                        if (!isHost)
                        {
                            quitListening = true;
                            return false;
                        }
                        else
                        {
                            continue;
                        }
                    }
                }

                //---------------------------------------------------Amount of connections Message?-------------------------------------------

                // Check if this is a message to check how many connections we have (non-host only)
                else if (!isHost && std::strncmp(listenBuffer, "Tot", 3 && std::strncmp(listenBuffer, "TotalConCount", 13)) == 0)
                {
                    int messResult = listenTotalConCount();
                    if (messResult == 1)
                    {
                        continue;
                    }
                }

                //---------------------------------------------------Keep Alive Message which is trash-------------------------------------------

                else if (std::strncmp(listenBuffer, "KeepAlive", 9) == 0)
                {
                    continue;
                }

                //---------------------------------------------------Package-Style Message?-------------------------------------------

                // Check if it is a sendTo message, if so, we know we are the host
                else if (std::strncmp(listenBuffer, "-To", 3) == 0)
                {
                    // Send to another client
                    if (std::strncmp(listenBuffer, "-ToCl-", 6) == 0)
                    {
                        int messResult = listenToClient();
                        if (messResult == 1)
                        {
                            continue;
                        }
                    }
                    // Handle it like a normal message but remove the first part
                    else if (isHost && std::strncmp(listenBuffer, "-ToHo", 5) == 0)
                    {
                        listenToHost(otherName);
                    }
                    // Send to all, including us but not to the sender
                    else if (std::strncmp(listenBuffer, "-ToAll", 6) == 0)
                    {
                        listenToAll(them_addr, otherName);
                    }
                }

                if (otherName.empty())
                {
                    int i = 0;
                    while (i < strlen(listenBuffer) && listenBuffer[i] != ':') otherName += listenBuffer[i++];
                }

                // If we are on the same device, we need to check if we are on the same program or not
                if (currentMoves == SAMEDEVICE && name == otherName)
                {
                    // we are the one sending the message, so ignore
                    // printf("Ignored above message\n");
                    continue;
                }

                // Only add to callback OR normal received storage, not to both. 
                std::function<void(const std::string&)>* function = nullptr;
                const std::string packageName = std::string(getWord(listenBuffer, 1));
                if (!packageName.empty())
                {
                    auto it = callBackFunctions.find(packageName);
                    if (it != callBackFunctions.end()) {
                        function = &(it->second);  // Get address of the function
                    }
                }

                // Store package to queue
                if (useCallBackFunctions && function != nullptr)
                {
                    callBackMutex.lock();
                    userPackageStorage.push(listenBuffer);
                    callBackMutex.unlock();
                }

                // Store message (No connection message)
                else if (!ConMessage) {

                    messageVectorMutex.lock();
                    receivedMessages.push_front(listenBuffer);
                    printf("Pushed Back Message: %s\n", listenBuffer);
                    // Delete oldest message if vector is too big
                    if (receivedMessages.size() > MaxMessagesVectorSize)
                    {
                        char errorBuffer[256];
                        sprintf_s(errorBuffer, sizeof(errorBuffer), "Message getting deleted due to being full: %s", receivedMessages.back().c_str());
                        LOGLINE(DLogObj, DanceLogger::DANCE_INFO, errorBuffer);
                        receivedMessages.pop_back();
                    }
                    messageVectorMutex.unlock();
                }


                if (!ConMessage) continue;  // Only do this next part if it is a connection message

                //---------------------------------------------------Do we have them already?-------------------------------------------

                // Convert from storage to addrinfo.
                addrinfo* res = storageToAddrInfo(them_addr, addrSize);


                // Now check if we already had this client
                bool gotThem = false;
                for (const auto& it : them_addrss)
                {
                    // Use 2 ways to check, one with the name and otherwise use the sa_data.
                    if (it.first == otherName)
                    {
                        gotThem = true;
                    }
                    else if (isSameAdress((sockaddr_storage*)res->ai_addr, (sockaddr_storage*)it.second->ai_addr))
                    {
                        gotThem = true;
                    }
                }
                if (!gotThem)
                {
                    printf("Welcome to the network: %s!\n", otherName.c_str());

                    // Add to map
                    them_addrss.emplace(make_pair(otherName, res));
                    them_numbers.emplace(make_pair(otherName, atPlayerNumber));
                    atPlayerNumber++;
                    //Send message of our new max connections
                    if (isHost)
                    {
                        totalConnections = int(them_addrss.size());
                        const char* ConAmountMessage = "TotalConCount ";
                        char ConAmountMessage2[50];
                        int size = snprintf(ConAmountMessage2, 50, "%s%i", ConAmountMessage, totalConnections);

                        // Iterate over all the other peers
                        for (const auto& it : them_addrss)
                        {
                            sendMessage(ConAmountMessage2, size, it.second, true, nullptr);
                        }
                    }

                    // Send a confirm that the message reached us succesfully, also give them their number 
                    // Note that we do not mark it as important, since this could cause an 'infinite' loop, better to have the other party send it twice.
                    std::string confirmMessage = "ConConfirm" + std::to_string(atPlayerNumber - 1);
                    sendMessage(confirmMessage.c_str(), int(confirmMessage.size()), res, false, nullptr);

                    // Now return if we dont want to keep checking
                    if (!keepChecking) return true;
                }

                //Hole Punching Check
                if (holePunchingStatus == 1) //listen() being called from holepunch()
                {
                    // Now check if we already had this client in the hole punching vector
                    gotThem = false;
                    for (int i = 0; i < int(holePunchConfirmedConnections.size()); i++)
                    {
                        if (isSameAdress((sockaddr_storage*)res->ai_addr, (sockaddr_storage*)holePunchConfirmedConnections[i]->ai_addr))
                        {
                            gotThem = true;
                        }
                    }
                    if (!gotThem)
                    {
                        totalConnections++;
                        holePunchConfirmedConnections.push_back(res);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

int Dance::listenImportantMessage(sockaddr_storage themAdrr, int addrSize)
{
    //Remove 'Imp ' from the buffer
    memcpy_s(listenBuffer, sizeof(listenBuffer), &listenBuffer[3], sizeof(listenBuffer) - 3);

    //Get User ID
    std::shared_ptr<char> user_ID = nullptr;
    getWord(listenBuffer, 0, user_ID);
    memcpy_s(listenBuffer,
        sizeof(listenBuffer),
        &listenBuffer[strlen(user_ID.get()) + 1],
        sizeof(listenBuffer) - (strlen(user_ID.get())));

    //Get message ID
    std::shared_ptr<char> message_ID = nullptr;
    getWord(listenBuffer, 0, message_ID);
    memcpy_s(listenBuffer,
        sizeof(listenBuffer),
        &listenBuffer[strlen(message_ID.get()) + 1],
        sizeof(listenBuffer) - (strlen(message_ID.get())));

    // Create ID based on number from received peer
    std::string fullID{};
    for (auto it : them_addrss)
    {
        if (isSameAdress(&themAdrr, (sockaddr_storage*)it.second->ai_addr))
        {
            fullID = std::to_string(them_numbers.at(it.first)) + std::string(message_ID.get()); 
        }
    }

    //Check if it is a long message
    bool long_full = true;
    if (std::strncmp(listenBuffer, "Long", 4) == 0)
    {
        //Remove 'Long' from buffer
        memcpy_s(listenBuffer, sizeof(listenBuffer), &listenBuffer[4], sizeof(listenBuffer) - 4);

        //Find Numbers
        std::string num_1;
        int num_2, idx_last;
        std::string number_finder(listenBuffer);
        number_finder = number_finder.substr(0, number_finder.find('/'));
        num_1 = number_finder;
        number_finder = listenBuffer;
        number_finder = number_finder.substr(number_finder.find('/') + 1,
            idx_last = int(number_finder.find(' ')));
        num_2 = std::stoi(number_finder);
        idx_last++;

        // Remove numbers from the buffer.
        memcpy_s(listenBuffer,
            sizeof(listenBuffer),
            &listenBuffer[idx_last],
            sizeof(listenBuffer) - idx_last);

        // Get Long Message ID
        std::shared_ptr<char> long_ID = nullptr;
        getWord(listenBuffer, 0, long_ID);
        memcpy_s(listenBuffer,
            sizeof(listenBuffer),
            &listenBuffer[strlen(long_ID.get()) + 1],
            sizeof(listenBuffer) - (strlen(long_ID.get())));

        // Add value to map, could use 'inserted' to check if emplaced or not.
        auto [it, inserted] = longMessageStorage.try_emplace(long_ID.get());
        auto& batchMapRet = it->second;
        // Also add message if it does not exists
        batchMapRet.try_emplace(num_1, listenBuffer);

        // Check if we got all. 
        if (batchMapRet.size() <= num_2) {
            // printf("Size is %d out of %d\n", num_2, batchMapRet.size());
            long_full = false;
        }
        else {
            //printf("Full message constructing\n");
            std::string fullMessage;
            fullMessage.reserve(num_2 * 1024);
            for (int i = 0; i < batchMapRet.size(); i++) {
                auto itBatchMessage = batchMapRet.find(std::to_string(i));
                if (itBatchMessage != batchMapRet.end()) {
                    fullMessage += itBatchMessage->second;
                }
                else {
                    // Something went wrong, full message is not complete.
                    long_full = false;
                    LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Not all batch messages were complete trying to recreate full message");
                    break;
                }
            }
            if (long_full) {
                char container[128];
                sprintf_s(container, "%s %s", user_ID.get(), message_ID.get());

                longMessageStorage.erase(container);
                memcpy_s(
                    listenBuffer, sizeof(listenBuffer), fullMessage.c_str(), fullMessage.size());
            }
        }
    }

    //Send back that it succeeded
    addrinfo* res = storageToAddrInfo(themAdrr, addrSize);
    char return_message[128];
    sprintf_s(return_message, "SImp%s %s", user_ID.get(), message_ID.get()); // Succes Important.
    sendMessage(return_message, int(strlen(return_message)), res, false, nullptr);

    // Check if we already handled this message, if we did not, add it to the set
    auto it = handledMessages.find(fullID);
    const bool foundMessage = (it != handledMessages.end());
    if (!foundMessage)
    {
        handledMessages.insert(fullID);
    }

    // We do not want to handle batch messages seperately, also check if we found message.
    if (!long_full || foundMessage)
    {
        return 1;
    }
    return 0;
}

int Dance::listenSuccesImportantMessage()
{
    //Remove 'SImp' from the buffer
    memcpy_s(listenBuffer, sizeof(listenBuffer), &listenBuffer[4], sizeof(listenBuffer) - 4);
    //Remove from array
    printf("Removed %s from important messages\n", listenBuffer);
    importantSendMessages.erase(listenBuffer);
    return 1;
}

int Dance::listenTotalConCount()
{
    // Update total connection count
    std::shared_ptr<char> buffer;
    getWord(listenBuffer, 1, buffer);
    totalConnections = std::atoi(buffer.get());
    return 1;
}

int Dance::listenToClient()
{
    // Support for 99 users
    int extra = 0;
    std::string ID = std::to_string(listenBuffer[6]);
    if (listenBuffer[7] != ' ')
    {
        ID += std::to_string(listenBuffer[7]);
        extra++;
    }
    const std::string message = &listenBuffer[8 + extra];
    sendMessageTo(message, std::stoi(ID), false, false, false);
    return 1;  // We dont want to do anything with this message so move on.
}

void Dance::listenToHost(std::string& otherName)
{
    // Fill in the name
    memcpy_s(listenBuffer, sizeof(listenBuffer), &listenBuffer[7], sizeof(listenBuffer) - 7);
    int i = 0;
    while (i < strlen(listenBuffer) && listenBuffer[i] != ':') otherName += listenBuffer[i++];
}

void Dance::listenToAll(sockaddr_storage themAdrr, std::string& otherName)
{
    std::string message = &listenBuffer[7];

    for (auto& it : them_numbers)
    {
        if (it.second != 0 &&
            !isSameAdress(&themAdrr, (sockaddr_storage*)them_addrss.at(it.first)->ai_addr))
        {
            sendMessageTo(message, it.second, false, false, false);
        }
    }
    // Now handle it ourself

    //  Fill in the name
    memcpy_s(listenBuffer, sizeof(listenBuffer), &listenBuffer[7], sizeof(listenBuffer) - 7);
    int i = 0;
    while (i < strlen(listenBuffer) && listenBuffer[i] != ':') otherName += listenBuffer[i++];
}

void Dance::checkOnImportantMessages()
{
    //Check on the other important messages in array
    if (!importantSendMessages.empty())
    {
        const auto ms = static_cast<unsigned long long>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
        const auto dt = static_cast<float>(ms - lastTime) * 0.001f;
        lastTime = ms;
        timeCheckedImp += dt;

        //Did enough time passed?
        if (timeCheckedImp > 0.1f)
        {
            timeCheckedImp = 0.0f;

            std::string eraseMessage{};
            for (auto& it : importantSendMessages)
            {
                if (it.second.checksDone >= 10)
                {
                    ////Probably do something
                    //char errorBuffer[256];
                    //sprintf_s(errorBuffer, sizeof(errorBuffer), "Never received important message confirmation from message: %s", it.second.message.c_str());
                    //LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
                    eraseMessage = it.first;
                }
                else //Send again
                {
                    printf("Sending again...\n");
                    //Get userID
                    std::shared_ptr<char> buffer = nullptr;
                    getWord(const_cast<char*>(it.second.ID.c_str()), 0, buffer);
                    const int userID = std::atoi(buffer.get());

                    char errorBuffer[256];
                    sprintf_s(errorBuffer, sizeof(errorBuffer), "Needed to send this important message a second time: %s", it.second.message.c_str());
                    LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, errorBuffer);

                    const std::string impReturnMessage = "Imp" + it.second.ID + " " + it.second.message; //Succes Important

                    for (const auto& pair : them_addrss)
                    {
                        if (them_numbers.at(pair.first) == userID)  // Send to this one
                        {
                            sendMessage(impReturnMessage.c_str(), int(impReturnMessage.size()), pair.second, false, nullptr);
                        }
                    }

                    it.second.checksDone++;
                }
            }
            if (!eraseMessage.empty())
            {
                importantSendMessages.erase(eraseMessage);
            }
        }
    }
}

// Using help from chatGPT
void Dance::sendCallBacks()
{
    quitCallBack = false;
    while (!quitCallBack)
    {
        while (!userPackageStorage.empty())
        {
            // Package name
            std::shared_ptr<char> packageName = nullptr;
            getWord(const_cast<char*>(userPackageStorage.front().c_str()), 1, packageName);

            // Find function that belongs to this package name
            if (packageName.get() != nullptr)
            {
                auto it = callBackFunctions.find(packageName.get());
                if (it != callBackFunctions.end())
                {
                    it->second(userPackageStorage.front());
                }
            }
            userPackageStorage.pop();
        }
    }
}

void Dance::resetState()
{
    ipv = AF_INET;
    totalConnections = 0;
    sockfd = 0;
    isHost = false;

    if (callBackThread.joinable())
    {
        quitCallBack = true;
        callBackThread.join();
    }
    holePunchingStatus = 0;
    currentMoves = SAMEDEVICE;

    maxConnections = 10;
    receivedMessages.clear();
    std::queue<std::string>().swap(userPackageStorage);
    callBackFunctions.clear();
    useCallBackFunctions = false;
    forceIPV4 = false;

    them_addrss.clear();
    them_numbers.clear();
    holePunchConfirmedConnections.clear();
}

bool Dance::isSameAdress(const sockaddr_storage* address1, const sockaddr_storage* address2)
{
    //Using help from Claude AI
    if (address1->ss_family != address2->ss_family)
    {
        return false;
    }

    //Check first IPV4
    if (address1->ss_family == AF_INET)
    {
        const sockaddr_in* a1 = (const sockaddr_in*)address1;
        const sockaddr_in* a2 = (const sockaddr_in*)address2;

        return (a1->sin_addr.s_addr == a2->sin_addr.s_addr) &&
            (a1->sin_port == a2->sin_port);
    }
    else if (address1->ss_family == AF_INET6)
    {
        //Else check IPV6
        const sockaddr_in6* a1 = (const sockaddr_in6*)address1;
        const sockaddr_in6* a2 = (const sockaddr_in6*)address2;
        
        return memcmp(&a1->sin6_addr, &a2->sin6_addr, sizeof(in6_addr)) == 0 &&
            a1->sin6_port == a2->sin6_port;
    }

    return false;
}

void Dance::sendMessage(const char* message, int message_size, addrinfo* adress, bool important, const char* send_error)
{
    // Check if message is too long to send, max is 1024, yet we check for 1000 to be able to cut off
    // a word correctly and add other info
    if (message_size > 1000) 
    {
        int amountMessages = int(std::ceil(float(message_size) / 964));
        int startIndex = 0;
        int atIndex = 964;

        // Find key
        int peerNumber = -1;
        for (const auto it : them_addrss) 
        {
            if (isSameAdress((sockaddr_storage*)adress->ai_addr, (sockaddr_storage*)them_addrss.at(it.first)->ai_addr))
            {
                peerNumber = them_numbers.at(it.first);
                break;
            }
        }

        for (int i = 0; i < amountMessages; i++) 
        {

            // printf("Sending message %d out of %d. \n", i, amountMessages);

            const int copyAmount = 964;

            char partMessage[copyAmount + 1];
            memcpy(partMessage, &message[startIndex], copyAmount);
            partMessage[copyAmount] = '\0'; /* Null terminate */

            // Add Long and Message ID 
            char formatMessage[64];
            sprintf_s(formatMessage, "Long%d/%d %d ", i, amountMessages - 1, atLongMessage);
            std::string seperateMessage = formatMessage + std::string(partMessage);

            // Make important
            addImportantMessage(seperateMessage, peerNumber);

            //printf("Sending message %d/%d\n", i, amountMessages - 1);

            // Send message. 
            if (sendto(sockfd,
                seperateMessage.c_str(),
                int(seperateMessage.size()),
                0,
                adress->ai_addr,
                int(adress->ai_addrlen)) == INVALID_SOCKET)
            {
                if (send_error == nullptr || strlen(send_error) > 200) 
                {
                    char errorBuffer[256];
                    sprintf_s(errorBuffer, sizeof(errorBuffer), "Send failed: %s", getSendError(WSAGetLastError()));
                    LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
                }
                else 
                {
                    LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, send_error);
                }
            }

            startIndex = atIndex;
            atIndex += copyAmount;
        }
        atLongMessage++;
    }
    /* If message is not too long, send it. */
    else {
        std::string messageString;
        if (important) {
            int peerNumber = -1;
            for (const auto it : them_addrss) {
                if (isSameAdress((sockaddr_storage*)adress->ai_addr, (sockaddr_storage*)them_addrss.at(it.first)->ai_addr))
                {
                    peerNumber = them_numbers.at(it.first);
                    break;
                }
            }
            messageString = message;
            addImportantMessage(messageString, peerNumber);
        }
        else {
            messageString = message;
        }

        if (sendto(sockfd,
            messageString.c_str(),
            int(messageString.size()),
            0,
            adress->ai_addr,
            int(adress->ai_addrlen)) == INVALID_SOCKET)
        {
            if (send_error == nullptr || strlen(send_error) > 200) {
                char errorBuffer[256];
                sprintf_s(errorBuffer, sizeof(errorBuffer), "Send failed: %s", getSendError(WSAGetLastError()));
                LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
            }
            else {
                LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, send_error);
            }
        }
    }
}

Dance::importantMesStruct Dance::addImportantMessage(std::string& message, const int toUserID)
{
    importantMesStruct iMessage;
    char ID_char[16];
    if (atImpMessage + 1 == UINT_MAX) atImpMessage = 1;
    sprintf_s(ID_char, "%i %d", toUserID, atImpMessage++);
    iMessage.ID = ID_char;
    iMessage.message = message;
    iMessage.checksDone = 0;
    importantSendMessages.emplace(iMessage.ID, iMessage);
    message = "Imp" + iMessage.ID + " " + message; iMessage;
    return iMessage;
}

bool Dance::handleDisconnection(sockaddr_storage input, int inputSize)
{
    //If we are not host, we just assume the peer has closed the connection, so we remove the client
    bool disconnected = false;
    // Check if we got this in our vector
    const addrinfo* res = storageToAddrInfo(input, inputSize);

    for (const auto& it : them_addrss)
    {
        if (isSameAdress((sockaddr_storage*)res->ai_addr, (sockaddr_storage*)it.second->ai_addr))
        {
            // remove from vector
            printf("Peer %s has disconnected...\n", it.first.c_str());
            disconnectedUserIDs.push_back(them_numbers.at(it.first));
            them_numbers.erase(it.first);
            them_addrss.erase(it.first);
            disconnected = true;

            //Update total amount of connections to all peers if host
            if (isHost)
            {
                totalConnections = int(them_addrss.size());
                const char* ConAmountMessage = "TotalConCount ";
                char ConAmountMessage2[50];
                const int size = snprintf(ConAmountMessage2, 50, "%s%i", ConAmountMessage, totalConnections);

                // Iterate over all the other peers
                for (const auto& it2 : them_addrss)
                {
                    sendMessage(ConAmountMessage2, size, it2.second);
                }
            }
            else
            {
                //Reset state, since our host was our everything
                resetState();
            }
            return disconnected;
        }
    }
    return disconnected;
}

std::string_view Dance::getWord(const std::string& input, int wordNumber)
{
    if (&input == nullptr || input.empty())
    {
        return {};
    }

    int EndOfWord = 0;
    int BeginOfWord = 0;

    // Get to the word
    int j = 0;
    for (int i = 0; i < wordNumber; i++)
    {
        while (input[j] != 32 && input[j] != '\0') j++;
        //Find next non-space
        while (input[j] == 32 && input[j] != '\0') j++;
    }
    // Get the word
    BeginOfWord = j;
    while (j < int(input.size()) && input[j] != 32 && input[j] != '\0') j++;
    EndOfWord = j;

    if (BeginOfWord != EndOfWord) return std::string_view(input.data() + BeginOfWord, EndOfWord - BeginOfWord);

    return {};
}

void Dance::getWord(char* input, int word_number, std::shared_ptr<char>& r_output)
{
    if (input == nullptr) {
        return;
    }

    //Get to the word.
    int j = 0;
    for (int i = 0; i < word_number; i++) {
        while (input[j] != 32 && input[j] != '\0') {
            j++;
        }
        /* Find next non-space. */
        while (input[j] == 32 && input[j] != '\0') {
            j++;
        }
    }
    //Get the word.
    const int begin_of_word = j;
    while (input[j] != 32 && input[j] != '\0') {
        j++;
    }
    const int end_of_word = j;

    if (begin_of_word != end_of_word) {

        //Create space and swap ownership (also reference count).
        std::shared_ptr<char> a(new char[end_of_word - begin_of_word + 1]);
        a.swap(r_output);

        memcpy_s(r_output.get(),
            end_of_word - begin_of_word,
            &input[begin_of_word],
            end_of_word - begin_of_word);

        r_output.get()[end_of_word - begin_of_word] = '\0';
    }
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//-----------------------------------------      Helper Functions       -----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

std::string Dance::getIP(bool publicIP)
{
    if (publicIP)
    {
        const std::string website_HTLM = get_Website();  // returns ipv4 or ipv6, whichever is possible
        if (website_HTLM != "0")
        {
            char OutputIP[65];
            strcpy_s(OutputIP, website_HTLM.c_str());  // Copy string to IP char
            return OutputIP;
            // Print
            // printf("Your IP adress = %s\n", website_HTLM.c_str());
        }
    }
    else
    {
        // Use the code from microsoft
        // https://learn.microsoft.com/nl-nl/windows/win32/api/iphlpapi/nf-iphlpapi-getadaptersinfo?redirectedfrom=MSDN
        PIP_ADAPTER_INFO pAdapterInfo;
        PIP_ADAPTER_INFO pAdapter = NULL;
        DWORD dwRetVal = 0;

        ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
        pAdapterInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
        if (pAdapterInfo == NULL)
        {
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Error allocating memory needed to call GetAdaptersinfo1");
        }
        // Make an initial call to GetAdaptersInfo to get
        // the necessary size into the ulOutBufLen variable
        if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
        {
            free(pAdapterInfo);
            pAdapterInfo = (IP_ADAPTER_INFO*)malloc(ulOutBufLen);
            if (pAdapterInfo == NULL)
            {
                LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Error allocating memory needed to call GetAdaptersinfo2");
            }
        }
        if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
        {
            pAdapter = pAdapterInfo;
            while (pAdapter)
            {
                std::string IP = pAdapter->IpAddressList.IpAddress.String;

                // Check if IP = 0.0.0.0
                if (IP == "0.0.0.0")
                {
                    pAdapter = pAdapter->Next;
                    continue;
                }

                // Check for 192.168.x.x, 10.x.x.x, and 172.16.x.x - 172.31.x.x
                //unsigned int a, b;
                //sscanf_s(IP.c_str(), "%u.%u", &a, &b);  // CHATGPT suggested this
                //if ((a == 192 && b == 168) || (a == 10) || (a == 172 && (b >= 16 && b <= 31)))
                //{
                //}
                //else
                //{
                //    // COULD BREAK IF ONLY A PUBLIC IP EXIST.
                //    pAdapter = pAdapter->Next;
                //    continue;  // Not a private IP.
                //}

                // Check for virtual machine by checking if they have a gateway
                if (std::string(pAdapter->GatewayList.IpAddress.String) == "0.0.0.0")
                {
                    pAdapter = pAdapter->Next;
                    continue;  // Virtual Machine
                }

                // This is the IP we need
                char OutputIP[65];
                strcpy_s(OutputIP, IP.c_str());  // Copy string to IP char
                return OutputIP;

                // If we want to Continue:

                // Print IP Adress
                // printf("\tIP Address: \t%s\n", IP.c_str());
                // pAdapter = pAdapter->Next;
                // printf("\n");
            }
        }
        else
        {
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "GetAdaptersInfo failed with error: %d\n", dwRetVal);
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        }
        if (pAdapterInfo) free(pAdapterInfo);
    }
    return "0";
}

std::string Dance::get_Website()
{
    // Credits from https://stackoverflow.com/a/39567361

    WSADATA wsaData;
    SOCKET Socket;
    SOCKADDR_IN6* SockAddr6;
    SOCKADDR_IN* SockAddr;
    std::string Website_HTLM;
    char buffer[10000];
    std::string url = "api64.ipify.org";
    const std::string get_http = "GET / HTTP/1.1\r\nHost: " + url + "\r\nConnection: close\r\n\r\n";


    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "WSASTARTUP failed trying to get the website");
        return "0";
    }

    struct addrinfo hints, * res, * p;
    int status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    ipv = AF_INET6;

    // Get address info of the site, note that port 80 is used: http
    // Comment to force ipv4
    if (forceIPV4 || (status = getaddrinfo(url.c_str(), "80", &hints, &res)) != 0)
    {
        // Try it with IPV4
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        ipv = AF_INET;
        url = "api.ipify.org";
        if ((status = getaddrinfo(url.c_str(), "80", &hints, &res)) != 0)
        {
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "error while getaddrinfo ourself: %s", getSendError(WSAGetLastError()));
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
            return "0";
        }
    }

    for (p = res; p != NULL; p = p->ai_next)
    {
        // IPV6
        if (hints.ai_family == AF_INET6)
        {
            SockAddr6 = (SOCKADDR_IN6*)p->ai_addr;
        }
        // IPV4
        else if (hints.ai_family == AF_INET)
        {
            SockAddr = (SOCKADDR_IN*)p->ai_addr;
        }
    }

    // Get socket
    Socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    // And connect
    if (connect(Socket, res->ai_addr, int(res->ai_addrlen)) != 0)
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Could not connect: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        return "0";
    }

    if (send(Socket, get_http.c_str(), int(strlen(get_http.c_str())), 0) == -1)  // Send request to site
    {
        char errorBuffer[256];
        sprintf_s(errorBuffer, sizeof(errorBuffer), "Could not send: %s", getSendError(WSAGetLastError()));
        LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
    }

    // Because we send a request to the site, it send some back to us with the information we need.
    // We unpack this information (very badly)
    int nDataLength;
    while ((nDataLength = recv(Socket, buffer, 10000, 0)) > 0)
    {
        int i = 0;
        while (buffer[i] >= 32 || buffer[i] == '\n' || buffer[i] == '\r')
        {
            // Checks for 2 new lines and a number after. Here is the IP
            if ((buffer[i] == '\n' || buffer[i] == '\r') && (buffer[i + 1] == '\n' || buffer[i + 1] == '\r') &&
                buffer[i + 2] >= 48 && buffer[i + 2] <= 57)
            {
                i += 2;
                while (buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] > 32)
                {
                    Website_HTLM += buffer[i];
                    i++;
                }
                break;
            }

            i += 1;
        }
    }

    freeaddrinfo(res);
    closesocket(Socket);
    WSACleanup();

    return Website_HTLM;
}

//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//----------------------------------------     Messages and Packages       --------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

void Dance::sendMessageTo(std::string message, int themID, bool toHost, bool toAll, bool important)
{
    if (isHost)
    {
        // Iterate of numbers
        for (const auto& pair : them_addrss)
        {
            if (toAll || them_numbers.at(pair.first) == themID)  // Send to this one
            {
                sendMessage(message.c_str(), int(message.size()), pair.second, important, nullptr);

                // Go back
                if (!toAll) return;
            }
        }
        if (!toAll)
        {
            // If here, themID is not available.
            LOGLINE(DLogObj, DanceLogger::DANCE_WARNING, "them ID is not found");
        }
        return;
    }
    else  // not host
    {
        if (toHost)  // Ignore themID, we send this one just to the host
        {
            const std::string DataMessage = "-ToHo " + message;

            // Send the message to host
            sendMessage(DataMessage.c_str(), int(DataMessage.size()), them_addrss.at("HOST"), important, nullptr);
        }
        else if (toAll)
        {
            const std::string DataMessage = "-ToAll " + message;

            // Send the message to host
            sendMessage(DataMessage.c_str(), int(DataMessage.size()), them_addrss.at("HOST"), important, nullptr);
        }
        else// Send to the host which will send it to the right user
        {
            const std::string DataMessage = "-ToCl-" + std::to_string(themID) + " " + message;

            // Send the message to host
            sendMessage(DataMessage.c_str(), int(DataMessage.size()), them_addrss.at("HOST"), important, nullptr);
        }
    }
}

bool Dance::isDisconnected(const int userID, bool remove)
{
    for (int i = 0; i < disconnectedUserIDs.size(); i++)
    {
        if (userID == disconnectedUserIDs.at(i))
        {
            if (remove) disconnectedUserIDs.erase(disconnectedUserIDs.begin() + i);
            return true;
        }
    }
    return false;
}

void Dance::sendPackage(std::string PackageName, bool important)
{
    memset(dataPackage, 0, MAXPACKAGESIZE * sizeof(char));

    if (isHost)
    {
        preparePackage(PackageName.c_str(), dataPackage);

        // Iterate over all the other peers
        for (const auto& it : them_addrss)            
        {
            //Don't send to host (ourself)
            if (them_numbers.find(it.first)->second == peer_ID) continue;

            //TODO: possibly just use a char
            std::string dataMessageString = dataPackage;

            sendMessage(dataMessageString.c_str(), int(dataMessageString.size()), it.second, important, nullptr);
        }
    }
    else  // not host
    {
        // First send to host which will send to all
        snprintf(dataPackage, MAXPACKAGESIZE, "-ToAll ");
        preparePackage(PackageName.c_str(), dataPackage + strlen(dataPackage));

        if (them_addrss.size() > 0)
        {
            std::string dataMessageString = dataPackage;
            sendMessage(dataMessageString.c_str(), int(dataMessageString.size()), them_addrss.at("HOST"), important, nullptr);
        }
    }
}

void Dance::sendPackageTo(std::string PackageName, int themID, bool ToHost, bool important)
{
    preparePackage(PackageName.c_str(), dataPackage);
    sendMessageTo(dataPackage, themID, ToHost, false, important);
}

void Dance::sendToSelf(std::string PackageName)
{
    preparePackage(PackageName.c_str(), dataPackage);
    messageVectorMutex.lock();
    receivedMessages.insert(receivedMessages.begin(), dataPackage);
    messageVectorMutex.unlock();
}

void Dance::preparePackage(const char* PackageName, char* DataMessage)
{
    // Get the data ready
    int offset = 0;
    memcpy(DataMessage + offset, name.c_str(), name.length());
    offset += int(name.length());
    std::memcpy(DataMessage + offset, ": ", 2);
    offset += 2;
    DataMessage[offset++] = ' ';
    memcpy(DataMessage + offset, PackageName, std::strlen(PackageName));
    offset += int(std::strlen(PackageName));
    DataMessage[offset++] = ' ';

    const auto& package = packageMap[PackageName];
    const int size = int(package.size());

    // Loop over all the data and add it to the message
    for (int i = 0; i < size; i++)
    {
        // Get value
        const std::variant<int, float, unsigned, const char*, std::string, double, bool>& Value = package[i];

        // Cast from correct type to string
        if (auto ValueInt = std::get_if<int>(&Value))
        {
            offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%i ", *ValueInt);
        }
        else if (auto ValueUnsigned = std::get_if<unsigned>(&Value))
        {
            offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%i ", *ValueUnsigned);
        }
        else if (auto ValueDouble = std::get_if<double>(&Value))
        {
            offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%f ", *ValueDouble);
        }
        else if (auto ValueFloat = std::get_if<float>(&Value))
        {
            offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%f ", *ValueFloat);
        }
        else if (auto ValueString = std::get_if<std::string>(&Value))
        {
            memcpy(DataMessage + offset, ValueString->c_str(), ValueString->length());
            offset += int(ValueString->length());
            DataMessage[offset++] = ' ';
        }
        else if (auto ValueChar = std::get_if<const char*>(&Value))
        {
            memcpy(DataMessage + offset, ValueChar, std::strlen(*ValueChar));
            offset += int(std::strlen(*ValueChar));
            DataMessage[offset++] = ' ';
        }
        else if (auto ValueBool = std::get_if<bool>(&Value))
        {
            offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%s ", *ValueBool ? "true" : "false");
        }
        //Message to big, report it and return
        if (offset > MAXPACKAGESIZE - 10)
        {
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Error, message size too big");
            return;
        }
    }
    offset += snprintf(DataMessage + offset, MAXPACKAGESIZE - offset, "%s", "end");

    return;
}


//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//----------------------------------------    Other helper functions     ----------------------------------------------
//---------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------

addrinfo* Dance::storageToAddrInfo(sockaddr_storage input, int inputSize)
{
    addrinfo* res;  // Result

    // If we are on the same device, we dont want to store the others based on the IP.
    if (currentMoves == SAMEDEVICE)
    {
        // Malloc memory for res
        res = reinterpret_cast<addrinfo*>(malloc(sizeof(addrinfo)));

        if (res == NULL)
        {
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Malloc failed");
            return nullptr;
        }
        // Manually copy the adress
        res->ai_family = input.ss_family;
        res->ai_socktype = SOCK_DGRAM;

        res->ai_addr = (struct sockaddr*)malloc(inputSize);
        if (res->ai_addr == NULL)
        {
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, "Malloc failed");
            return nullptr;
        }
        memcpy(res->ai_addr, &input, inputSize);
        res->ai_addrlen = inputSize;
    }
    else
    {
        struct addrinfo hints;
        int status;
        char addr_str[INET6_ADDRSTRLEN] = { 0 };  // This will hold the IP

        if (input.ss_family == AF_INET)  // IPV4
        {
            // Get IP of other
            struct sockaddr_in* ipv4 = (struct sockaddr_in*)&input;
            inet_ntop(AF_INET, &ipv4->sin_addr, addr_str, sizeof(addr_str));
        }
        else  // IPV6
        {
            // Get IP of other
            struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)&input;
            inet_ntop(AF_INET6, &ipv6->sin6_addr, addr_str, sizeof(addr_str));
        }

        // Handle this and put it inside the map
        memset(&hints, 0, sizeof hints);
        hints.ai_family = input.ss_family;
        hints.ai_socktype = SOCK_DGRAM;
        // addrinfo
        if ((status = getaddrinfo(addr_str, hostPort, &hints, &res)) != 0)
        {
            char errorBuffer[256];
            sprintf_s(errorBuffer, sizeof(errorBuffer), "getaddrinfo error on clients IP: %s", getSendError(WSAGetLastError()));
            LOGLINE(DLogObj, DanceLogger::DANCE_ERROR, errorBuffer);
        }
    }

    return res;
}



const char* Dance::getSendError(int errorCode)
{
    const char* output;

    switch (errorCode)
    {
    case 10022:
        output = "Invalid Argument (10022)\n";
        break;
    case 10038:
        output = "Socket was invalid\n";
        break;
    case 10048:
        output = "Address already in use\n";
        break;
    case 10050:
        output = "Network is down\n";
        break;
    case 10051:
        output = "Network is unreachable\n";
        break;
    case 10054:
        output = "Connection reset by peer\n";
        break;
    case 10060:
        output = "Connection Timed Out\n";
        break;
    case 10061:
        output = "Connection Refused\n";
        break;
    default:
        char buffer[256];

        snprintf(buffer, 256, "(errorCode: %i not implemented, view https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2 for more information\n", errorCode);
        output = buffer;
        break;
    }
    return output;
}