#include "sprite.h"
#include "ui.h"

#include "opengl.h"
#include "input.hpp"
#include "spritetransform.h"
#include "common.h"






void ui::setNextSize(float scaleX, float scaleY, int width, int height)
{
	nextScale = { scaleX, scaleY }; //TODO: could look weird with some values, test and fix
	nextWidthHeight = { width, height };
	NextSizeSet = true;
}

void ui::setNextWorldSpace()
{
	NextWorldSpace = true;
}

bool ui::inputText(const char* label, const char** output, glm::vec2 pos, const char* setTextTo, bool emptyText)
{
	if (screenObj != nullptr)
	{
		float scaleX = nextScale.x;

		Entity spriteEntity = firstChecks(label, pos, true, "NONE", InputText);


		spriteStr& Sprite = Registry.get<spriteStr>(spriteEntity);
		UILabel& UI = Registry.get<UILabel>(spriteEntity);


		//Check if clicked or hovered
		glm::vec2 mousepos = input::Input().mousePos;
		if (!UI.screenSpace) mousepos = input::Input().getMousePosOffset();
		mousepos.y = SCRHEIGHT - mousepos.y;
		if (mousepos.x > pos.x - Sprite.getAfterWidth() * 0.5f && mousepos.x < pos.x + Sprite.getAfterWidth() * 0.5f &&
			mousepos.y > pos.y - Sprite.getAfterHeight() * 0.5f && mousepos.y < pos.y + Sprite.getAfterHeight() * 0.5f)
		{
			//Hovered
			if (input::Input().getMouseButtonOnce(input::MouseButton::Left))
			{
				//Clicked 
				UI.status = true;
			}
			else
			{
				sprite::setBrightenSprite(&Sprite, 25);
			}
		}
		else
		{
			sprite::setBrightenSprite(&Sprite, 0);
			if (input::Input().getMouseButtonOnce(input::MouseButton::Left))
			{
				//Clicked Outside
				UI.status = false;
			}
		}

		//Handle text input
		if (UI.status && input::Input().keysChangedAmount > 0)
		{
			//Check what changed
			std::string keysPressed;
			bool backspace = false;

			for (int i = 0; i < input::Input().keysChangedAmount; i++)
			{
				unsigned key = input::Input().keystateChanged[i];
				if (key == 259)backspace = true; //Backspace
				else if (input::Input().getKeyDownOnce(static_cast<input::Key>(key)))
				{
					if ((key >= 65 && key <= 90) || (key >= 39 && key <= 57) || key == 32) //Limit to numbers and letters (also some characters)
					{
						keysPressed += static_cast<unsigned char>(key); //ASCII 
					}
				}
			}
			
			if (backspace && !UI.inputText.empty()) UI.inputText.pop_back();
			else if (UI.inputText.size() + keysPressed.size() < MAXINPUTTEXT) UI.inputText += keysPressed;
		}
		if (setTextTo != 0) UI.inputText = setTextTo;

		//Update visuals
		if ((UI.active && !UI.inputText.empty() && input::Input().keysChangedAmount > 0) || emptyText || setTextTo != 0)
		{
			if (emptyText) UI.inputText = "";//TODO: make a function apart for emptying
			if (UI.inputTextSprite.name == "None") sprite::initEmpty(&UI.inputTextSprite, 1, "InputTextSprite", CHARACTERSIZE * MAXINPUTTEXT, CHARACTERSIZE);
			else sprite::emptySprite(&UI.inputTextSprite, glm::vec4(0));

			//Check how many characters fit in sprite
			int lettersFit = int((float(Sprite.getAfterWidth()) - 6 * scaleX) * INVCHARACTERSIZE * 0.5f); 
			//View only last n amount of letters
			std::string_view textView = &UI.inputText[std::max(0,int(UI.inputText.size()) - lettersFit)];

			screenObj->ReceivePrint(textView.data(), UI.inputTextSprite.buffer, CHARACTERSIZE * MAXINPUTTEXT, 4294967295);
			//Make text just a little bigger
			UI.inputTextSprite.localSpriteTransform.setScale({ 2.0f,2.0f }); //TODO: scale up letters when both next x and y scale.
			//Now we need to update the buffer
			sprite::updateIndividualSpriteBuffer(UI.inputTextSprite);

			*output = UI.inputText.c_str(); //Output only changes when value changes, TODO: maybe change?

			return true; //Value changed
		}

	}
	else
	{
		printf("ScreenObj not initialized\n");
	}
	return false;
}

void ui::text(const char* label, const char* text, glm::vec2 pos, unsigned color)
{
	if (screenObj != nullptr)
	{
		//Set also scale of text
		glm::vec2 sizeIncrease = nextScale;

		Entity spriteEntity = firstChecks(label, pos, true, "NONE", Text);

		Stransform& transform = Registry.get<Stransform>(spriteEntity);
		UILabel& UI = Registry.get<UILabel>(spriteEntity);

		bool textChanged = false;

		//Handle text input
		if (UI.active && UI.inputText != text && strlen(text) < MAXINPUTTEXT)
		{
			textChanged = true;
			UI.inputText = text;
		}
		else if (strlen(text) >= MAXINPUTTEXT)
		{
			printf("Text is longer than MAX text lenght, change max lenght or shorten text\n");
		}

		//Offset sprite transform
		glm::vec2 newPos = pos;
		newPos.x += float(CHARACTERSIZE * MAXINPUTTEXT) * sizeIncrease.x * 0.5f; //Set to middle
		newPos.x -= 0.5f * sizeIncrease.x * float(CHARACTERSIZE * int(UI.inputText.size())); //Offset by each new character

		transform.setTranslation(newPos);

		//Update visuals
		if (UI.active && !UI.inputText.empty() && (newPos != transform.getTranslation() || textChanged))
		{
			if (UI.inputTextSprite.name == "None") sprite::initEmpty(&UI.inputTextSprite, 1, "TextSprite", CHARACTERSIZE * MAXINPUTTEXT, CHARACTERSIZE);
			else sprite::emptySprite(&UI.inputTextSprite, glm::vec4(0));


			screenObj->ReceivePrint(UI.inputText.c_str(), UI.inputTextSprite.buffer, CHARACTERSIZE * MAXINPUTTEXT, color);
			//Maybe change size 
			UI.inputTextSprite.localSpriteTransform.setScale(sizeIncrease);

			//Now we need to update the buffer
			sprite::updateIndividualSpriteBuffer(UI.inputTextSprite);

		}
	}
}

bool ui::button(const char* label, glm::vec2 pos, bool defaultLooks, const char* image)
{
	//First check if we already loaded this button
	Entity spriteEntity = firstChecks(label, pos, defaultLooks, image, Button);
	


	spriteStr& Sprite = Registry.get<spriteStr>(spriteEntity);
	UILabel& UI = Registry.get<UILabel>(spriteEntity);


	//Check if clicked or hovered
	glm::vec2 mousepos = input::Input().mousePos;
	if (!UI.screenSpace) mousepos = input::Input().getMousePosOffset();
	mousepos.y = SCRHEIGHT - mousepos.y;
	if (mousepos.x > pos.x - Sprite.getAfterWidth() * 0.5f && mousepos.x < pos.x + Sprite.getAfterWidth() * 0.5f &&
		mousepos.y > pos.y - Sprite.getAfterHeight() * 0.5f && mousepos.y < pos.y + Sprite.getAfterHeight() * 0.5f)
	{
		//Hovered
		if (input::Input().getMouseButtonOnce(input::MouseButton::Left))
		{
			//Clicked
			sprite::setBrightenSprite(&Sprite, -25);
			return true;
		}
		else
		{
			sprite::setBrightenSprite(&Sprite, 25);
		}
	}
	else
	{
		sprite::setBrightenSprite(&Sprite, 0);
	}
	return false;
}

bool ui::checkBox(const char* label, bool& output, glm::vec2 pos, bool defaultLooks, const char* image)
{
	//First check if we already loaded this button
	Entity spriteEntity = firstChecks(label, pos, defaultLooks, image, CheckBox);

	spriteStr& Sprite = Registry.get<spriteStr>(spriteEntity);
	UILabel& UI = Registry.get<UILabel>(spriteEntity);

	//Check if clicked or hovered
	glm::vec2 mousepos = input::Input().mousePos;
	if (!UI.screenSpace) mousepos = input::Input().getMousePosOffset();
	mousepos.y = SCRHEIGHT - mousepos.y;
	if (mousepos.x > pos.x - Sprite.getAfterWidth() * 0.5f && mousepos.x < pos.x + Sprite.getAfterWidth() * 0.5f &&
		mousepos.y > pos.y - Sprite.getAfterHeight() * 0.5f && mousepos.y < pos.y + Sprite.getAfterHeight() * 0.5f)
	{
		//Hovered
		if (input::Input().getMouseButtonOnce(input::MouseButton::Left))
		{
			//Clicked
			UI.status = UI.status ? false : true;
			sprite::setFrameSprite(&Sprite, int(UI.status));
			output = UI.status;
			return true;
		}
		else
		{
			sprite::setBrightenSprite(&Sprite, 25);
		}
	}
	else
	{
		sprite::setBrightenSprite(&Sprite, 0);
	}
	return false;
}


bool ui::radioButton(const char* label, int& output, int numberOutput, glm::vec2 pos, bool defaultLooks, const char* image)
{
	//First check if we already loaded this button
	Entity spriteEntity = firstChecks(label, pos, defaultLooks, image, RadioButton);

	spriteStr& Sprite = Registry.get<spriteStr>(spriteEntity);
	UILabel& UI = Registry.get<UILabel>(spriteEntity);

	UI.radioButtonPointer = &output;

	//Check if clicked or hovered
	glm::vec2 mousepos = input::Input().mousePos;
	if (!UI.screenSpace) mousepos = input::Input().getMousePosOffset();
	mousepos.y = SCRHEIGHT - mousepos.y;
	if (mousepos.x > pos.x - Sprite.getAfterWidth() * 0.5f && mousepos.x < pos.x + Sprite.getAfterWidth() * 0.5f &&
		mousepos.y > pos.y - Sprite.getAfterHeight() * 0.5f && mousepos.y < pos.y + Sprite.getAfterHeight() * 0.5f)
	{
		//Hovered
		if (input::Input().getMouseButton(input::MouseButton::Left))
		{
			//Clicked

			if (UI.radioButtonTarget != spriteEntity)
			{
				//Check if valid
				if (static_cast<int>(UI.radioButtonTarget) != 0)
				{
					//Notify target
					spriteStr& targetSprite = Registry.get<spriteStr>(UI.radioButtonTarget);
					sprite::setFrameSprite(&targetSprite, 0);
				}

				//Loop over all radiobuttons to check which ones are part of this series
				std::map<std::string, Entity>::iterator uiEntity;
				for (uiEntity = SavedSprites.begin(); uiEntity != SavedSprites.end(); uiEntity++)
				{
					auto& UITarget = Registry.get<UILabel>(uiEntity->second);
					if (UITarget.radioButtonPointer == &output)
					{
						//Part of group
						UITarget.radioButtonTarget = spriteEntity;
					}
				}


				//Change target
				UI.radioButtonTarget = spriteEntity;
				sprite::setFrameSprite(&Sprite, 1);
				output = numberOutput;
				return true;
			}

		}
		else
		{
			sprite::setBrightenSprite(&Sprite, 25);
		}
	}
	else
	{
		sprite::setBrightenSprite(&Sprite, 0);
	}
	return false;
}

void ui::image(const char* label, glm::vec2 pos, const char* image, unsigned layer, int MHX, int MHY)
{
	Entity spriteEntity = firstChecks(label, pos, false, image, Image, layer);

	if (MHX != 0 || MHY != 0)
	{
		Stransform& transform = Registry.get<Stransform>(spriteEntity);
		spriteStr& Sprite = Registry.get<spriteStr>(spriteEntity);

		if (MHX > 0) MHX = 1;
		else if (MHX < 0) MHX = -1;
		if (MHY > 0) MHY = 1;
		else if (MHY < 0) MHY = -1;
		glm::vec2 offset = { Sprite.getAfterWidth() * 0.5f * MHX, Sprite.getAfterHeight() * 0.5f * MHY };
		transform.setTranslation(pos + offset);
	}

}


void ui::update(Shader* screenShader, Shader* worldShader)
{
	//if (button("Test", { 1100,350 }, true, " ")) printf("Button 1 pressed\n");
	//if (button("Test2", { 1150,350 }, true, " ")) printf("Button 2 pressed\n");
	//if (button("Test3", { 1200,350 }, true, " ")) printf("Button 3 pressed\n");
	//
	//static int output = 0;
	//if (radioButton("R1", output, 5, { 1100, 300 })) printf("RadioButton 1 selected, output is now: %i\n", output);
	//if (radioButton("R2", output, 6, { 1150, 300 })) printf("RadioButton 2 selected, output is now: %i\n", output);
	//if (radioButton("R3", output, 7, { 1200, 300 })) printf("RadioButton 3 selected, output is now: %i\n", output);
	//
	//static bool C1Output = false;
	//static bool C2Output = false;
	//static bool C3Output = false;
	//if (checkBox("C1", C1Output, { 1100, 400 })) printf("CheckBox 1 selected, output is now: %d\n", C1Output);
	//if (checkBox("C2", C2Output, { 1150, 400 })) printf("CheckBox 2 selected, output is now: %d\n", C2Output);
	//if (checkBox("C3", C3Output, { 1200,400 })) printf("CheckBox 3 selected, output is now: %d\n", C3Output);
	//
	//const char* output2 = "test";
	//if (inputText("Input1", &output2, { 1100, 500 }))
	//{
	//	printf(output2);
	//	printf("\n");
	//}
	//setNextSize(3, 3);
	//text("text1", "Hey You!", { 1100, 550 }, 4278255615);


	//Draw buttons that are active;

	for (int i = 0; i < int(drawOrderUI.size()); i++)
	{
		UILabel& UI = Registry.get<UILabel>(drawOrderUI[i]);

		if (UI.active)
		{
			spriteStr& Sprite = Registry.get<spriteStr>(drawOrderUI[i]);
			Stransform& transform = Registry.get<Stransform>(drawOrderUI[i]);
			Shader* shader = screenShader;
			if (!UI.screenSpace) shader = worldShader;

			if (Sprite.bufferChanged)
			{
				Sprite.texture->CopyFrom(Sprite.afterBuffer);
				Sprite.bufferChanged = false;
			}
			shader->Bind();
			shader->SetInputTexture(0, "c", Sprite.texture);
			DrawQuadSprite(shader, transform, Sprite.texture, &Sprite);
			shader->Unbind();

			//InputText
			if (!UI.inputText.empty())
			{
				if (UI.inputTextSprite.bufferChanged)
				{
					UI.inputTextSprite.texture->CopyFrom(UI.inputTextSprite.afterBuffer);
					UI.inputTextSprite.bufferChanged = false;
				}
				shader->Bind();
				shader->SetInputTexture(0, "c", UI.inputTextSprite.texture);
				//Does use the transform of parent
				glm::vec2 pos = transform.getTranslation();
				if (UI.type == 4) transform.setTranslation({ (4 * Sprite.localSpriteTransform.getScale().x) + pos.x + float(UI.inputTextSprite.width) - float(Sprite.getAfterWidth()) * 0.5f, pos.y });
				DrawQuadSprite(shader, transform, UI.inputTextSprite.texture, &UI.inputTextSprite);
				transform.setTranslation(pos);
				shader->Unbind();
			}

		}
	}

}

void ui::updateLogic()
{
	for (int i = 0; i < int(drawOrderUI.size()); i++)
	{
		UILabel& UI = Registry.get<UILabel>(drawOrderUI[i]);

		if (UI.active)
		{
			UI.active = false; //Only draw UI that is called 
		}
	}
}

Entity ui::firstChecks(std::string label, glm::vec2 pos, bool defaultLooks, const char* image, UIType Type, unsigned layer)
{
	Entity outputEntity;

	std::map<std::string, Entity>::const_iterator uiEntity = SavedSprites.find(label);
	if (uiEntity == SavedSprites.end())
	{
		//Load sprite
		outputEntity = Registry.create();
		spriteStr& Sprite = CreateComponent<spriteStr>(outputEntity);
		Stransform& transform = CreateComponent<Stransform>(outputEntity);
		UILabel& UIL = CreateComponent<UILabel>(outputEntity);
		UIL.active = true;
		UIL.label = label; //Copy value so its not pointer.
		UIL.type = static_cast<int>(Type);
		UIL.layer = layer;
		transform.setTranslation(pos);

		std::string file = "assets/ui/";
		file += typeToFile(Type);
		int frames = 1;
		if (Type == ui::RadioButton || Type == ui::CheckBox) frames = 2;

		if (defaultLooks) sprite::initFromFile(&Sprite, file.c_str(), frames, "DefaultButton");
		else sprite::initFromFile(&Sprite, image, frames, label.c_str());

		if (NextWorldSpace) UIL.screenSpace = false;
		else UIL.screenSpace = true;

		if (NextSizeSet)
		{
			if (nextScale != glm::vec2(1)) Sprite.localSpriteTransform.setScale(nextScale);
			else sprite::setSpriteWidthHeight(&Sprite, int(nextWidthHeight.x), int(nextWidthHeight.y));
		}
		SavedSprites.emplace(label, outputEntity);

		//Draw order
		bool found = false;
		for (int i = 0; i < int(drawOrderUI.size()); i++)
		{
			if (layer < Registry.get<UILabel>(drawOrderUI[i]).layer)
			{
				drawOrderUI.insert(drawOrderUI.begin() + i, outputEntity);
				found = true;
				break;
			}
		}
		if (!found) drawOrderUI.push_back(outputEntity);

	}
	else
	{
		outputEntity = uiEntity->second;
		spriteStr& Sprite = Registry.get<spriteStr>(outputEntity);
		UILabel& UIL = Registry.get<UILabel>(outputEntity);
		UIL.active = true;

		if (NextSizeSet && (nextScale != Sprite.localSpriteTransform.getScale() || (Sprite.getAfterWidth() != nextWidthHeight.x && Sprite.getAfterHeight() != nextWidthHeight.y)))
		{
			if (nextScale != glm::vec2(1)) Sprite.localSpriteTransform.setScale(nextScale);
			else sprite::setSpriteWidthHeight(&Sprite, int(nextWidthHeight.x), int(nextWidthHeight.y));
		}
	}
	NextWorldSpace = false;
	NextSizeSet = false;
	nextScale = { 1,1 };
	nextWidthHeight = { 0,0 };

	return outputEntity;
}

const char* ui::typeToFile(UIType Type)
{
	switch (Type)
	{
	case ui::Button:
		return "defaultButton.png";
		break;
	case ui::RadioButton:
		return "defaultRadioButton.png";
		break;
	case ui::CheckBox:
		return "defaultCheckBox.png";
		break;
	case ui::InputText:
		return "defaultInputText.png";
		break;
	case ui::Text:
		return "defaultText.png";
		break;
	default:
		return "NONE";
		break;
	}
	return nullptr;
}
