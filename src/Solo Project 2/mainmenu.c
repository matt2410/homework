#include "cprocessing.h"
#include "mainmenu.h"
#include "game.h"
#include "utils.h"

// Text alignment settings used for menu button labels.
CP_TEXT_ALIGN_HORIZONTAL horizontal = CP_TEXT_ALIGN_H_CENTER;
CP_TEXT_ALIGN_VERTICAL vertical = CP_TEXT_ALIGN_V_MIDDLE;

// Font used by the main menu.
CP_Font text;
float red;
float blue;
float green;
/*
 * Initializes resources needed by the main menu.
 * Loads the font and sets the default text size.
 */
void Main_Menu_Init(void)
{
	text = CP_Font_Load("Assets/Exo2-Regular.ttf");
	CP_Settings_TextSize(32);
}

/*
 * Updates and draws the main menu every frame.
 * Handles drawing the buttons, their labels, and mouse input.
 */
void Main_Menu_Update(void)
{
	// Clear the screen with a light gray background.
	CP_Graphics_ClearBackground(CP_Color_Create(200, 200, 200, 255));

	// Set a white outline around the menu buttons.
	CP_Settings_Stroke(CP_Color_Create(255, 255, 255, 255)); 
	CP_Settings_StrokeWeight(2.0f);

	// Use the loaded font for all menu text.
	CP_Font_Set(text);

	// -------------------------
	// PLAY BUTTON
	// -------------------------
	
	// Set the Play button's fill color to blue.
	CP_Settings_Fill(CP_Color_Create(red, blue, green, 255));

	// Draw the Play button in the upper-middle portion of the screen.
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, CP_System_GetWindowHeight() / 3.0f, 250.0f, 150.0f);


	// Set the Play button's fill color to blue.
	CP_Settings_Fill(CP_Color_Create(0, 128, 255, 255)); 

	// Draw the Play button in the upper-middle portion of the screen.
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, CP_System_GetWindowHeight() / 3.0f, 250.0f, 150.0f);

	// Center the "Play" text inside the button.
	CP_Settings_TextAlignment(horizontal, vertical);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 255));

	CP_Font_DrawText("Play", CP_System_GetWindowWidth() / 2, CP_System_GetWindowHeight() / 3.0);

	// -------------------------
	// EXIT BUTTON
	// -------------------------
	
	// Set the Exit button's fill color to dark red.
	CP_Settings_Fill(CP_Color_Create(190, 0, 0, 255));
	
	// Draw the Exit button in the lower-middle portion of the screen.
	CP_Graphics_DrawRect(CP_System_GetWindowWidth() / 2.0f, CP_System_GetWindowHeight() * 2.0f / 3.0f, 250.0f, 150.0f);

	// Center the "Exit" text inside the button.
	CP_Settings_TextAlignment(horizontal, vertical);
	CP_Settings_Fill(CP_Color_Create(0, 0, 0, 255));

	CP_Font_DrawText("Exit", CP_System_GetWindowWidth() / 2, CP_System_GetWindowHeight() * 2 / 3.0);

	CP_Settings_NoStroke();
	CP_Settings_Fill(CP_Color_Create(255, 128, 50, 255));

	float CP_Input_GetMouseX(void);
	float CP_Input_GetMouseY(void);
	if (CP_Input_MouseTriggered(MOUSE_BUTTON_LEFT))
	{
		if (IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, CP_System_GetWindowHeight() / 3.0f, 250.0f, 150.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()) == 1)
		{
			CP_Engine_SetNextGameState(Game_Init, Game_Update, Game_Exit);
		}
		if (IsAreaClicked(CP_System_GetWindowWidth() / 2.0f, CP_System_GetWindowHeight() * 2.0f / 3.0f, 250.0f, 150.0f, CP_Input_GetMouseX(), CP_Input_GetMouseY()) == 1)
		{
			CP_Engine_Terminate();
		}
	}
}

void Main_Menu_Exit(void)
{
	CP_Font_Free(text);
}