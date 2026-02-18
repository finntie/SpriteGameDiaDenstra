#pragma once

#include <string>
#include <queue>
#include <chrono>
#include <ctime>
#include <assert.h>

#ifdef _DEBUG
#define LOGLINE(DanceLogObj, severity, msg) DanceLogger::logWarning((DanceLogObj), (severity), (msg), __FILE__, __LINE__)
#define POPUPLOGBOX(message) { MessageBoxA(NULL, message, "Oh bummer, a mishap occured", MB_OK | MB_ICONERROR); exit(1); }
#else
#define LOGLINE(DanceLogObj, severity, msg) (void(0))
#endif

#ifdef _DEBUG

class DanceLogger
{
public:

    enum severityEnum
    {
        DANCE_NONE, DANCE_CRITICAL, DANCE_ERROR, DANCE_WARNING, DANCE_INFO
    };

    struct logMessage
    {
        logMessage(severityEnum _severe, const char* _message, const char* _file, int line, std::string _time)
            : logSevere(_severe), message(_message), file(_file), lineNumber(line), time(_time)
        {
        }
        ~logMessage() {}

        severityEnum logSevere;
        const char* message;
        const char* file;
        int lineNumber;
        std::string time; //String due to const char* going out of scope after creation
    };


    /// <summary>Function called from the macro LOGLINE, logs the message and possibly give an error if set to</summary>
    /// <param name="DObj">Class object to store it in</param>
    /// <param name="severity">How severe is the issue</param>
    /// <param name="msg">Message that relates to the issue</param>
    /// <param name="file">At which file did it happen (generated from the macro)</param>
    /// <param name="lineNumber">At which line did it happen (generated from the macro)</param>
    static void logWarning(DanceLogger& DLogObj, severityEnum severity, const char* msg, const char* file, int lineNumber)
    {
        if (severity == DANCE_NONE) return; //TODO: should I just return?

        //Add info to the logger and check if we already need to break for this issue
        DLogObj.logger.push_back(logMessage(severity, msg, file, lineNumber, getCurrentTime()));
        checkBreakSeverity(DLogObj.logger[static_cast<int>(DLogObj.logger.size()) - 1], DLogObj.breakSeverity);
    }


    /// <summary>Set minimum severity the program should break, if set to DANCE_ERROR, the program will break when DANCE_ERROR or DANCE_CRITICAL occured.</summary>
    /// <param name="severity">severity enum</param>
    void setBreakSeverity(severityEnum severity)
    {
        breakSeverity = severity;
    }


    /// <summary>Print all issues at a severity</summary>
    /// <param name="atSeverity">Minimum severity of issues to print</param>
    /// <param name="only">Print only issues from set severity</param>
    /// <param name="breakProgram">Break with a pop-up at the end</param>
    void printIssues(severityEnum atSeverity, bool only, bool breakProgram, bool deleteIssue = false)
    {
        if (atSeverity == DANCE_NONE) return;

        bool oneIssueIsFound{ false };

        //Loop over all issues
        for (auto it = logger.begin(); it != logger.end();)
        {
            //Check if severity is met
            if ((only && (it->logSevere == atSeverity)) || (!only && (static_cast<int>(it->logSevere) <= static_cast<int>(atSeverity))))
            {
                sendIssueMessage(*it);
                oneIssueIsFound = true;

                if (deleteIssue)
                {
                    it = logger.erase(it);
                    continue;
                }
            }
            ++it;
        }

        //Breaks program if issue is found with the correct message
        if (breakProgram && oneIssueIsFound)
        {
            std::string issueMessage{};
            const char* severities[5] = { "NONE", "CRITICAL", "ERROR", "WARNING", "INFO" };
            if (static_cast<int>(atSeverity) < 5) //Check
            {
                if (only)
                {
                    issueMessage = "One or more issues with the severity " + std::string(severities[static_cast<int>(atSeverity)]) + " is found\n";
                }
                else
                {
                    issueMessage = "One or more issues with the severity " + std::string(severities[static_cast<int>(atSeverity)]) + " or higher is found\n";
                }
            }

            POPUPLOGBOX(issueMessage.c_str());
        }
    }


private:

    /// <summary>Checks if program should break based on severity</summary>
    /// <param name="lMSG">Log message containing all info</param>
    /// <param name="breakS">At which severity it should break</param>
    static void checkBreakSeverity(logMessage lMSG, severityEnum breakS)
    {
        if (breakS != DANCE_NONE && lMSG.logSevere != DANCE_NONE && static_cast<int>(lMSG.logSevere) <= static_cast<int>(breakS))
        {
            const std::string fullMessage = sendIssueMessage(lMSG);

            //Popup, break and exit when pressed 'ok'
            POPUPLOGBOX(fullMessage.c_str());
        }
    }


    /// <summary>Prepares and sends the issue message</summary>
    /// <param name="lMSG">logging message</param>
    /// <returns>full message but without the severity in the beginning</returns>
    static std::string sendIssueMessage(logMessage lMSG)
    {
        //Can never be NONE
        const char* severities[5] = { "NONE", "CRITICAL", "ERROR", "WARNING", "INFO" };
        const std::string fullMessage = std::string(lMSG.message) + " \nin: " + lMSG.file + " at line " + std::to_string(lMSG.lineNumber) + " at time: " + lMSG.time;

        //Print message in correct color
        setColor(getColorSeverity(lMSG.logSevere));
        printf("[%s]", severities[static_cast<int>(lMSG.logSevere)]);
        resetColor();
        printf(" %s\n", fullMessage.c_str());
        return fullMessage;
    }


    /// <summary>Calculates the current time in hours,minutes,seconds and miliseconds </summary>
    /// <returns>String of the time</returns>
    static std::string getCurrentTime()
    {
        std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        auto currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        struct tm newTime;
        //https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/localtime-s-localtime32-s-localtime64-s?view=msvc-170
        char timebuf[26];
        localtime_s(&newTime, &currentTime);
        strftime(timebuf, 26, "%H:%M:%S", &newTime);
        std::string time = std::string(timebuf) + "." + std::to_string(ms.count() % 1000);
        return time;
    }


    /// <summary>Makes text the color of input</summary>
    /// <param name="color">input color, use colors from: https://learn.microsoft.com/en-us/windows/console/console-screen-buffers#character-attributes, example: FOREGROUND_RED </param>
    static void setColor(int color)
    {      
        //https://learn.microsoft.com/en-us/windows/console/setconsoletextattribute
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, color);
    }
    /// <summary>Resets text color to be white</summary>
    static void resetColor()
    {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    }
    /// <summary>Get correct color per severity for text console</summary>
    /// <param name="severity">Severity</param>
    /// <returns>int to pass into setColor()</returns>
    static int getColorSeverity(severityEnum severity)
    {
        switch (severity)
        {
        case DanceLogger::DANCE_CRITICAL:
            //Dark red
            return FOREGROUND_RED;
            break;
        case DanceLogger::DANCE_ERROR:
            //Bright red
            return FOREGROUND_RED | FOREGROUND_INTENSITY;
            break;
        case DanceLogger::DANCE_WARNING:
            //Yellow
            return FOREGROUND_RED | FOREGROUND_GREEN;
            break;
        case DanceLogger::DANCE_INFO:
            //BLUE
            return FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        default:
            //white
            return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            break;
        }
    }


    //Variables
    std::deque<logMessage> logger;
    severityEnum breakSeverity{ DANCE_ERROR }; //At which point should the program break?
};

#else

//TODO: fix for if more functions are used to be called outside of this class
class DanceLogger
{
public:
    enum severityEnum
    {
        DANCE_NONE, DANCE_CRITICAL, DANCE_ERROR, DANCE_WARNING, DANCE_INFO
    };

    void setBreakSeverity(severityEnum ) { return; }
    void printIssues(severityEnum, bool, bool, bool = false) { return; }

private:
};


#endif