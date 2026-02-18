#include <glm.hpp>
#include <vector>

#include "common.h"
#include "ui.h"

class worldStats
{

public:

	//Physics class object
	static worldStats& WorldStats()
	{
		static worldStats classObject;
		return classObject;
	}

	struct uiMessage
	{
		char messageID[16];
		std::string message{};
		float durationLeft = 1.0f;
	};

	void init(int playersAmount, std::vector<std::string> playerNames)
	{
		
		for (int i = 0; i < playersAmount; i++)
		{
			allTabListData data{};
			data.name = playerNames[i];
			tabListData.push_back(data);
		}

		PossibleAssists.resize(playersAmount);
		for (int i = 0; i < playersAmount; i++)
		{
			PossibleAssists[i].resize(playersAmount);
		}
	}

	void showUI(float dt)
	{
		int yOffset = 0;
		for (int i = int(messageList.size()) - 1; i >= 0; i--)
		{
			char labelImage[16];
			sprintf_s(labelImage, sizeof(labelImage), "Image%d", i);
			ui::UI().image(labelImage, { 164.0f, SCRHEIGHT - yOffset - 128 }, "assets/uiSprites/textBackground.png", 10, 0, 0);

			ui::UI().setNextSize(2.0f, 2.0f);
			ui::UI().text(messageList[i].messageID, messageList[i].message.c_str(), { 164.0f, SCRHEIGHT - yOffset - 128 });
			yOffset += 32;
		}

		//Handle data
		for (int i = int(messageList.size()) - 1; i >= 0; i--)
		{
			if ((messageList[i].durationLeft -= dt) <= 0.0f)
			{
				//Remove
				messageList.erase(messageList.begin() + i);
			}
		}
	}

	struct allTabListData
	{
		std::string name;
		int playerKills{ 0 };
		int playerGotKilled{ 0 };
		int playerDamageDone{ 0 };
		int playerDamageReceived{ 0 };
		int playerAssists{ 0 };
	};

	std::vector<allTabListData> tabListData;
	std::vector<std::vector<bool>> PossibleAssists{};
	std::vector<uiMessage> messageList{};

	const char* tabList = "Player - KILLS  - ASSIST - DEATHS - DAMAGE - DMGREC";

	void playerKilled(int playerKills, int playerGotKilled, int bulletType)
	{
		tabListData[playerKills].playerKills++;
		tabListData[playerGotKilled].playerGotKilled++;
		AssistHappened(playerKills, playerGotKilled);
		//Create ui message
		uiMessage message{};
		message.durationLeft = 5.0f;
		sprintf_s(message.messageID, sizeof(message.messageID), "killUI%d", playerKills + playerGotKilled * 16);
		message.message = tabListData[playerKills].name + " (" + std::to_string(playerKills) + ")" + " killed " + tabListData[playerGotKilled].name + " (" + std::to_string(playerGotKilled) + ")";
		messageList.push_back(message);
	}

	void playerDidDamage(int player, int playerReceived, int amount)
	{
		tabListData[player].playerDamageDone += amount;
		tabListData[playerReceived].playerDamageReceived += amount;
		PossibleAssists[player][playerReceived] = true;
	}

	void showAllData()
	{
		ui::UI().image("tabListImage", {SCRWIDTH * 0.5f, SCRHEIGHT * 0.5f}, "assets/uiSprites/backgroundMenu.png", 11, 0, 0);


		int yOffset = 32;
		int xOffset = 0;
		const int offsetXAmount = 105;
		ui::UI().setNextSize(2, 2);
		ui::UI().text("tabList", tabList, { SCRWIDTH * 0.5f, SCRHEIGHT * 0.7f });

		for (int i = 0; i < tabListData.size(); i++)
		{
			// Name
			std::string message = tabListData[i].name + " (" + std::to_string(i) + ")";
			std::string label = "tabListName" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });
			xOffset += offsetXAmount;

			// Kills
			message = std::to_string(tabListData[i].playerKills);
			label = "tabListKills" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });
			xOffset += offsetXAmount;

			// Assists
			message = std::to_string(tabListData[i].playerAssists);
			label = "tabListAssists" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });
			xOffset += offsetXAmount;

			// Got Killed
			message = std::to_string(tabListData[i].playerGotKilled);
			label = "tabListGotKilled" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });
			xOffset += offsetXAmount;

			// Damage Done
			message = std::to_string(tabListData[i].playerDamageDone);
			label = "tabListDamageDone" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });
			xOffset += offsetXAmount;

			// Damage Received
			message = std::to_string(tabListData[i].playerDamageReceived);
			label = "tabListDamageReceived" + std::to_string(i);
			ui::UI().setNextSize(2, 2);
			ui::UI().text(label.c_str(), message.c_str(), { SCRWIDTH * 0.3f + xOffset, SCRHEIGHT * 0.7f - yOffset });


			xOffset = 0;
			yOffset += 32;
		}
	}

private:

	void AssistHappened(int playerKills, int playerGotKilled)
	{
		//First reset our player
		PossibleAssists[playerKills][playerGotKilled] = false;
		//Now check and reset others
		for (int i = 0; i < int(PossibleAssists.size()); i++)
		{
			if (i == playerKills) continue;
			if (PossibleAssists[i][playerGotKilled])
			{
				tabListData[i].playerAssists++;
				PossibleAssists[i][playerGotKilled] = false;
			}
		}
	}
};