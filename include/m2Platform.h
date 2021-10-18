
/*
 *
 *
 * Menu2 v2.0
 * Martin Hubacek
 * 18.3.2013
 * http://martinhubacek.cz
 *
 *
 */

#include "application.h"

#define MENU_LANGUAGES 1

// If you use different languages, define them here
#define LANGUAGE_CZECH 0
#define LANGUAGE_ENGLISH 1

unsigned char menuLanguage;

// Set rows/cols based on your font (for graphical displays)
#define ROW(x) ((x)*13)
#define COL(y) ((y)*6)
// For character LCDs use definitions below
//#define ROW(x) (x)
//#define COL(y) (y)

// Number of items on one screen
// Not including title
#define MENU_LINES 8

// Symbol which is displayed in front of the selected item
// This symbol doesn't appear when MENU_LINES == 1
#define ARROW_SYMBOL ">"
// How many spaces is between arrow symbol and menu item
// useful to set zero on smaller displays
#define ARROW_GAP 2

// Clear display
#define displayClear()	twr_module_lcd_clear()

// Display string
#define displayString(str, posx, posy) lcdBufferString(str, posx, posy)
// If you have separate functions for set position and print text, use define below
//#define displayString(str, posx, posy) {lcdGotoXY(posx, posy); lcdBufferString(str);}

// Display number
#define displayNumber(str, posx, posy) lcdBufferNumber(str, posx, posy)
// If you have separate functions for set position and print number, use define below
//#define displayNumber(str, posx, posy) {lcdGotoXY(posx, posy); lcdBufferNumber(str)}

// Optional function to write buffer to display - comment if not used
#define displayDraw()		twr_module_lcd_update()

#define BTN_UP 1
#define BTN_DOWN 2
#define BTN_LEFT 3
#define BTN_RIGHT 4
#define BTN_ENTER 5
