//Extra helper functions

#include <string>

std::string_view getWordDot(std::string& input, int wordNumber);
const char charToNumber(const char number, int section);
const char numberToChar(const char number, int section);

//Could be a bit expensive when called too much
static std::string IPToCode(std::string& IP)
{
	std::string output;

    std::string_view firstColon = getWordDot(IP, 0);
    std::string_view secondColon = getWordDot(IP, 1);
    std::string_view thirdColon = getWordDot(IP, 2);
    std::string_view fourthColon = getWordDot(IP, 3);

    int sectionNumber = 2;

    for (int i = 0; i < int(thirdColon.size()); i++)
    {
        output += charToNumber(thirdColon[i], sectionNumber++ % 3);
    }
    output += " ";
    for (int i = 0; i < int(fourthColon.size()); i++)
    {
        output += charToNumber(fourthColon[i], sectionNumber++ % 3);
    }
    output += " ";
    if (firstColon == "192") output += "1O";
    else if (firstColon == "255") output += "3M";
    else output += firstColon;
    output += " ";
    if (secondColon == "168") output += "1P";
    else if (secondColon == "255") output += "3M";
    else output += secondColon;

    return output;
}


static std::string CodeToIP(std::string& Code)
{
    std::string output;

    std::string_view firstColon = getWordDot(Code, 0);
    std::string_view secondColon = getWordDot(Code, 1);
    std::string_view thirdColon = getWordDot(Code, 2);
    std::string_view fourthColon = getWordDot(Code, 3);


    if (thirdColon == "1O") output += "192";
    else if (thirdColon == "3M") output += "255";
    else output += thirdColon;
    output += ".";
    if (fourthColon == "1P") output += "168";
    else if (fourthColon == "3M") output += "255";
    else output += fourthColon;

    int sectionNumber = 2;
    output += ".";

    for (int i = 0; i < int(firstColon.size()); i++)
    {
        output += numberToChar(firstColon[i], sectionNumber++ % 3);
    }
    output += ".";
    for (int i = 0; i < int(secondColon.size()); i++)
    {
        output += numberToChar(secondColon[i], sectionNumber++ % 3);
    }

    return output;
}


std::string_view getWordDot(std::string& input, int wordNumber)
{
    int EndOfWord = 0;
    int BeginOfWord = 0;

    // Get to the word
    int j = 0;
    for (int i = 0; i < wordNumber; i++)
    {
        while (input[j] != 46 && input[j] != 32 && input[j] != '\0') j++;
        //Find next non-dot
        while ((input[j] == 46 || input[j] == 32) && input[j] != '\0') j++;
    }
    // Get the word
    BeginOfWord = j;
    while (j < int(input.size()) && input[j] != 46 && input[j] != 32 && input[j] != '\0') j++;
    EndOfWord = j;

    if (BeginOfWord != EndOfWord) return std::string_view(input.data() + BeginOfWord, EndOfWord - BeginOfWord);

    return {};
}

const char charToNumber(const char number, int section)
{
    int numberConvert = (number);
    numberConvert -= 48; // 0 now is 0, not 48
    numberConvert += section * 10;
    numberConvert = numberConvert > 25 ? numberConvert - 26 : numberConvert;
    numberConvert += 65; //To ASCII
    return numberConvert;
}

const char numberToChar(const char number, int section)
{
    int numberConvert = (number - 65);
    numberConvert -= section * 10;
    numberConvert += 48; //To ASCII
    return numberConvert;
}