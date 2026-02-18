#pragma once

#include "register.hpp"
#include "screen.h"
#include <glm.hpp>
#include <map>

#define MAXINPUTTEXT 64
#define CHARACTERSIZE 6
#define INVCHARACTERSIZE (1.0f/float(CHARACTERSIZE))

struct spriteStr;
class Shader;
class ui
{
public:

	//UI class object
	static ui& UI()
	{
		static ui classObject;
		return classObject;
	}

	void setScreen(screen* Screen) { screenObj = Screen; }

	//Set size of the next item, scale or just set width and height
	void setNextSize(float scaleX, float scaleY, int width = 0, int height = 0);

	void setNextWorldSpace();

	bool inputText(const char* label, const char** output, glm::vec2 pos, const char* setTextTo = 0, bool emptyText = false);

	//Takes alpha into account
	void text(const char* label, const char* text, glm::vec2 pos, unsigned color = 4294967295);

	//Create a button at position, checks if button is pressed
	bool button(const char* label, glm::vec2 pos, bool defaultLooks = true, const char* image = "NONE");


	bool checkBox(const char* label, bool& output, glm::vec2 pos, bool defaultLooks = true, const char* image = "NONE");

	/// <summary>To create multiple checkbox with only one being able to be checked</summary>
	/// <param name="label">Name</param>
	/// <param name="output">Return value, will not be changed if not checked</param>
	/// <param name="numberOutput">What the output value will be when this button is checked</param>
	/// <param name="pos">Pos</param>
	/// <param name="defaultLooks">Normal looks or Self-made image</param>
	/// <param name="image">If not default looks, what image?</param>
	/// <returns>If value changed to true</returns>
	bool radioButton(const char* label, int& output, int numberOutput, glm::vec2 pos, bool defaultLooks = true, const char* image = "NONE");

	/// <summary>Draw custom image</summary>
	/// <param name="label">Name</param>
	/// <param name="pos">Position</param>
	/// <param name="image">Image directory</param>
	/// <param name="movehalfX">Do you want to move it half right or left? (1 or -1)</param>
	/// <param name="movehalfY">Do you want to move it half up or down? (1 or -1)</param>
	void image(const char* label, glm::vec2 pos, const char* image, unsigned layer = 0, int movehalfX = 0, int movehalfY = 0);

	//Draw and update UI
	void update(Shader* screenShader, Shader* worldShader);

	// Update the draw logic of the UI, this should be done just before calling the UI elements.
	void updateLogic();

private:
	ui() {};

	struct UILabel
	{
		std::string label;
		bool active = false;
		int type = 0;
		bool status = false; //Activated or not
		bool screenSpace = true; //Screen or worldSpace?

		int* radioButtonPointer = nullptr; //Points at output int.
		Entity radioButtonTarget{};

		std::string inputText = "";
		spriteStr inputTextSprite{};

		unsigned int layer = 0;
	};

	enum UIType
	{
		Button, RadioButton, CheckBox, Image, InputText, Text
	};

	Entity firstChecks(std::string label, glm::vec2 pos, bool defaultLooks, const char* image, UIType Type, unsigned layer = 9999);

	const char* typeToFile(UIType Type);

	std::map<std::string, Entity> SavedSprites;
	std::vector<Entity> drawOrderUI; //Could be a map or just an array

	glm::vec2 nextScale = { 1,1 };
	glm::vec2 nextWidthHeight = { 0,0 };
	bool NextSizeSet = false;
	bool NextWorldSpace = false;
	screen* screenObj = nullptr;

};

