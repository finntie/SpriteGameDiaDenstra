#pragma once
#define NOMINMAX  // Get rid of issues with MIN and MAX in winsock
#include <winsock2.h>

#include <map>
#include <vector>
#include <variant>
#include <string>
#include <sstream>
#include <atomic>
#include <future>
#include <deque>
#include <mutex>
#include <queue>

#define MAXPACKAGESIZE 1024

class Dance
{
public:

    enum DanceMoves
    {
        SAMEDEVICE,
        LAN,
        PUBLIC
    };

    Dance(){};
    ~Dance();

    /// <summary>Initializes the networking, always call this first</summary>
    /// <param name="UseCallBack">Do you want to use callback functions?</param>
    /// <param name="ForceIPV4">Use IPV4 always, else it will try IPV6 if possible (Will cause connection issues if other player does not match IPV)</param>
    void init(bool UseCallBack, bool ForceIPV4);

    //-----Engine Functions----------------


    /// <summary>Host a server for you and your friends</summary>
    /// <param name="moves">Are the programs on the same device? Or is it LAN (Same network different device)? Or is it public
    /// (Other network)</param> <param name="MaxConnections">Maximum clients, 10 is default</param> <param name="Port">Port
    /// number, for now leave it at 0, (leaving it at 0 will cause it grabbing a default port number).</param> <param
    /// name="clientsIP">If chosen for public network, the host has to fill in all the clients IPs</param>
    void Host(DanceMoves moves,
              int MaxConnections,
              const char* Port = 0,
              const std::vector<std::string>& clientsIP = std::vector<std::string>());

    /// <summary>Connect to a server</summary>
    /// <param name="moves">Are the programs on the same device? Or is it LAN (Same network different device)? Or is it public
    /// (Other network)</param>
    /// <param name="hostIP">The IP of the host (server) this only needs to be filled in if a public connection needs to be
    /// made. Otherwise it does not matter what is filled in</param>
    void Connect(DanceMoves moves, char* hostIP, const char* Port = 0);

    /// <summary>If we are using hole punching to connect to another client, 
    /// this connection has to stay alive. Thus calling KeepAlive every minute is nessecary.</summary>
    void KeepAlive(float deltaTime);

    /// <summary>Retrieve the IP of this device, !!Also sets the IP type (IPV4/IPV6)!!</summary>
    /// <param name="publicIP">Do we want the public or private IP?</param>
    std::string getIP(bool publicIP);

    /// <summary>Sent a message to specified ID or to the host</summary>
    /// <param name="Message">Message you want to send</param>
    /// <param name="themID">ID you want to send it to. 0 = host.</param>
    /// <param name="OnlyToHost">Do you want to send the message to the host? (Ignores themID)</param>
    void sendMessageTo(std::string Message, int themID, bool OnlyToHost);

    /// <summary>Before calling this function calculateTotalConnections() must have been called.
    /// Does not include our own connection</summary>
    int getTotalConnections() { return TotalConnections; }

    /// <summary>Are we the host?</summary>
    bool getIfHost() { return host; }

    ///< summary>Returns if ipv4 is used or ipv6</summary>
    bool getUseIPV4() { return (ipv == 2); }

    ///< summary>Set manually ipv</summary>
    /// <param name="IPV">2 == IPV4, 23 == IPV6</param>
    /// <param name="ForceIPV4">Forves IPV4 even if we have IPV6</param>
    void setIPV(int IPV, bool ForceIPV4)
    {
        ipv = IPV;
        forceIPV4 = ForceIPV4;
    }

    //--------------------------------------------Package Functions----------------------------------------------

    /// <summary>Create a package, you may already fill it with some variables</summary>
    /// <param name="PackageName">Name of the package</param>
    /// <param name="args">Variables you want to add, such as 1.0f or true, or 5, etc.</param>
    template <typename... Args>
    void CreatePackage(std::string PackageName, Args... args)
    {
        // Making a vector with all the parameters the user has put in.
        std::vector<std::variant<int, float, unsigned, const char*, std::string, double, bool>> InputVector{args...};
        PackageMap.emplace(std::make_pair(PackageName, InputVector));
    }
    /// <summary>Adds variables to this package</summary>
    /// <param name="PackageName">Name of the package</param>
    /// <param name="args">Variables you want to add, such as 1.0f or true, or 5, etc.</param>
    template <typename... Args>
    void AddParametersToPackage(std::string PackageName, Args... args)
    {
        PackageMap[PackageName].insert(PackageMap[PackageName].end(), {args...});
    }
    /// <summary>Change the data of the selected variable</summary>
    /// <param name="PackageName">Name of the package</param>
    /// <param name="VariableIndex">Index of the variable you want to change in order of when added</param>
    /// <param name="Data">Variable data</param>
    template <typename T>
    void AddDataToParameter(std::string PackageName, int VariableIndex, T Data)
    {
        PackageMap[PackageName][VariableIndex] = Data;
    }
    /// <summary>Deletes the variable from the package</summary>
    /// <param name="PackageName">Name of the package</param>
    /// <param name="number">Index of the variable you want to remove</param>
    /// <param name="all">Ignores the number and just deletes all variables in this package</param>
    void DeleteParameterFromPackage(std::string PackageName, int number, bool all)
    {
        if (all)
        {
            PackageMap[PackageName].clear();
        }
        else
        {
            PackageMap[PackageName].erase(PackageMap[PackageName].begin() + number);
        }
    }

    /// <summary>Create a callback function for a certain package. This function will be called when this package
    /// arrived</summary> <param name="PackageName">Name of the package you want to receive a call on.</param> <param
    /// name="function">Function that you created that will handle this package.</param>
    void CreatePackageCallBackFunction(std::string PackageName, std::function<void(const std::string& data)> function)
    {
        CallBackFunctions.emplace(std::make_pair(PackageName, function));
    }

    //-----------------------------------------PACKAGES-----------------------------------------------

    /// <summary>Send the created package to all connections</summary>
    /// <param name="PackageName">Name of the package</param>
    void sendPackage(std::string PackageName);

    /// <summary>Send a package to an individual user based on order of connections,
    /// if we are not the host, this will first send it to the host which then will decide where it will go
    /// (internally)</summary> <param name="PackageName">Name of the package</param> <param name="themID">ID of the client, this
    /// order is defined by the host (0 == host)</param> <param name="OnlyToHost">Only send package to host</param>
    void sendPackageTo(std::string PackageName, int themID, bool OnlyToHost);

    /// <summary>Send a package to ourself (Add it straight to the incomming messages)</summary>
    /// <param name="PackageName">Name of the package</param>
    void sendToSelf(std::string PackageName);

    /// <summary>Prepares the package to string</summary>
    /// <param name="PackageName">Name of the package</param>
    void preparePackage(const char* PackageName, char* buffer);

    struct ReturnPackageInfo
    {
        bool succeeded = false;
        std::vector<std::variant<int, float, unsigned, const char*, std::string, double, bool>> VariableVector;
        template <typename T>
        T getVariable(int Index)
        {
            if (auto* value = std::get_if<T>(&VariableVector[Index]))
                return *value;
            else 
            {
                //TODO: how would the user know at run time to change get type?
                printf("Error getVariable(): Wrong type\n");
                return T{};
            }
        }
        //TODO: add timestamp
        //std::string TimeStamp;
    };

    //ChatGPT recommendation
    template <typename T>
    bool tryParse(std::string& input, T& output)
    {
        std::stringstream ssTmp(input);
        ssTmp >> std::boolalpha >> output;  // Use std::boolalpha to handle "true"/"false" strings for bool
        return !ssTmp.fail() && ssTmp.eof(); //Check if succeeded
    }

    /// <summary>Get package that we received, dont forget to add <int> or <bool> based on what you want</summary>
    /// <param name="PackageName">Name of the package</param>
    /// <param name="DeleteMessage">Delete the message after we are done getting it?</param>
    /// <param name="packageInfo">Struct that will fill with a bool that states if the action was successfull + a vector of all types</param>
    void getPackage(std::string PackageName, bool DeleteMessage, ReturnPackageInfo* packageInfo)
    {
        MessageVectorMutex.lock();

        bool isList = false;
        for (int i = int(ReceivedMessages.size()) - 1; i >= 0; --i)
        {
            bool gotName = false;
            std::string tmp;
            int wordIndex = 0;
            while (!(tmp = getWord(ReceivedMessages[i], wordIndex++)).empty())
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
                packageInfo->succeeded = true; //Temporarely to true

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

                // This is the variable we are looking for
                //if (VariableIndex == wordIndex - 3)  // 1 for name, 1 for listname, 1 for placement of currentIndex++
                //{
                //    *succeeded = true;
                //    std::stringstream ssTmp(tmp);
                //    T value{};
                //    ssTmp >> value;  // Put the string into the value which will be converted.
                //    if (DeleteMessage)
                //    {
                //        ReceivedMessages.erase(ReceivedMessages.begin() + i);  // Delete this message if we want to.
                //    }
                //    MessageVectorMutex.unlock();
                //    return value;
                //}
            }
            //Gathered all variables (hopefully)
            if (DeleteMessage && packageInfo->succeeded)
            {
                ReceivedMessages.erase(ReceivedMessages.begin() + i);  // Delete this message if we want to.
            }
            if (isList) break;
        }
        // Could not find data
        // printf("Could not find data specified\n");
        MessageVectorMutex.unlock();
    }

    template <typename T>
    T dataToVariable(std::string& data, int VariableIndex)
    {
        std::string tmp;
        int wordIndex = 0;
        while (!(tmp = getWord(data, wordIndex++)).empty())
        {
            // This is the variable we are looking for
            if (VariableIndex == wordIndex - 1)
            {
                std::stringstream ssTmp(tmp);
                T value{};
                ssTmp >> value;  // Put the string into the value which will be converted.
                return value;
            }
        }
        return T{};
    }

private:
    // Functions

    /// <summary>Listens for incomming messages, acts uppon what to do with them. Thread function that will repeat forever until
    /// told to.</summary> <param name="KeepChecking">If set to true, we will quit when receiving a connection confirm message
    /// or a new client connected</param>
    bool listen(bool KeepChecking);

    /// <summary>Keeps sending messages to all connections until connection is made</summary>
    void holePunch();

    /// <summary>Sends the package to all the callbacks as thread</summary>
    bool sendCallBacks();

    // Helper functions

    /// <summary>Converts sockaddr_storage to addrinfo*</summary>
    /// <param name="input">Information of a user you want to convert</param>
    /// <param name="inputSize">Size of the input</param>
    addrinfo* storageToAddrInfo(sockaddr_storage input, int inputSize);

    /// <summary>Returns your public IP</summary>
    // Credit: https://stackoverflow.com/a/39567361
    std::string get_Website();

    /// <summary>Get word out of a string, returns empty when invalid</summary>
    /// <param name="input">Input string</param>
    /// <param name="wordNumber">Word you want to get, starts at 0</param>
    std::string_view getWord(std::string& input, int wordNumber);

    /// <summary>Convert error code to string</summary>
    static const char* getSendError(int errorCode);

    // IP information
    int ipv = AF_INET;  // AF_INET or AF_INET6 for ipv4 or ipv6
    int TotalConnections = 0;

    // Winsock Variables
    SOCKET sockfd{};

    // Device Information
    std::string name = "NONE";
    std::string DeviceName = "0";
    char HOSTPORT[5] = "8392";
    bool host = false;
    std::atomic<bool> QuitListening = false;  // Will only be set by the main thread exiting
    std::atomic<bool> QuitCallBack = false;   // Will only be set by the main thread exiting
    int HolePunchingStatus = 0;
    DanceMoves CurrentMoves = SAMEDEVICE;

    // Other important information
    float KeepAliveTime = 60.0f;
    int MaxConnections = 10;  // Standard is 10
    std::deque<std::string> ReceivedMessages{};
    std::queue<std::string> UserPackageStorage{};
    std::map<std::string, std::function<void(const std::string&)>> CallBackFunctions;
    std::future<bool> ConnectionMade;
    std::future<bool> CallBackReturn;
    bool UseCallBackFunctions = false;
    bool forceIPV4 = false;
    // thread safety
    std::mutex MessageVectorMutex;
    std::mutex CallBackMutex;

    // Map of Data Lists, which holds the data. //Data types can be expanded.
    std::map<std::string, std::vector<std::variant<int, float, unsigned, const char*, std::string, double, bool>>> PackageMap{};

    // Map of others, key = name
    std::map<std::string, struct addrinfo*> them_addrss{};
    // Vector of connections that are confirmed.
    std::vector<struct addrinfo*> HolePunchConfirmedConnections{};
};