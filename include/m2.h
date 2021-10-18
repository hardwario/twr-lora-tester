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


#ifndef __MENU2__
#define __MENU2__

#include "m2Platform.h"

// 0-3 bits
#define MENU_PARAMETER_MASK 0x0F
#define MENU_PARAMETER_IS_NUMBER 1
#define MENU_PARAMETER_IS_STRING 2

// 4. bit checkbox bit
#define MENU_ITEM_IS_CHECKBOX	0x10
// 5bit
#define MENU_ITEM_IS_CHECKED	0x20

// 6.bit - submenu bit
#define MENU_CALLBACK_IS_SUBMENU	0x40

// 7bit - callback bit
#define MENU_CALLBACK_IS_FUNCTION 0x80

typedef struct SMenuItem MenuItem;
typedef struct SMenu Menu;

struct SMenuItem {
	char* text[MENU_LANGUAGES];
	struct Menu *submenu;
	int flags;
	void *parameter;
};

struct SMenu {
	char* title[MENU_LANGUAGES];
	int selectedIndex;
	int refresh;

    int menuItem;
    int lastMenuItem;

    int cursorTopPos;
	int menuTopPos;

    int len;
    twr_tick_t refreshTimer;

    void (*callback)(Menu*, MenuItem*);

    struct Menu *menu_previous;
    struct Menu *menu_next;

    MenuItem *items[];
};

int menu2_init(Menu *menu);
int menu2_event(Menu *menu, uint8_t keyPress);
int menu2_draw(Menu *menu);

#endif
