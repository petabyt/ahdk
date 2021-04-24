#include "header.h"
#include "ambarella.h"
#include "mainmenu.h"

#ifndef FONTSIZE
	#define FONTSIZE 2737
#endif

// Global env
int *envg;
int line;
int sel;
struct Font font[100];

// General purpose buffer
char buffer[100];

void notify() {
	drawBox(10, 10, 200, 40, MENU_COL);
	printString(20, 20, buffer, TEXT_COL);
}

void countdown(int sec) {
	for (; sec != 0; sec--) {
		amb_sprintf(buffer, "Waiting %d...", sec);
		notify();
		amb_msleep(1000);
	}
}

void print(char *string) {
	printString(20, 20 + (line * 14), string, TEXT_COL);
	line++;
}

// Draw main UI box
void drawGUI() {
	drawBox(10, 10, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 40, MENU_COL);
}

void drawMenu(struct MenuItem menu[]) {
	// Menu selector
	int y = 0;
	for (int i = 0; menu[i].text != 0; i++) {
		int col;
		if (sel == i) {
			col = BTN_DOWN_COL;
		} else {
			col = BTN_UP_COL;
		}

		fillRect(20, 20 + y, SCREEN_WIDTH - 30, 40 + y, col);
		printString(26, 26 + y, menu[i].text, TEXT_COL);

		// Print
		if (menu[i].type == SELECT) {
			printString(SCREEN_WIDTH - 120, 26 + y, menu[i].info->elements[menu[i].info->s], TEXT_COL);
		}

		y += 22;
	}
}

// Return GPIO status
int gpioStat(int id) {
	int b, c, d;
	amb_gpio(id, &b, &c, &d);
	return !(c & 0xff);
}

// Wait until a button is pressed
int waitButton(int id) {
	while (!gpioStat(id)) {
		amb_msleep(1);
	}

	while (gpioStat(id)) {
		amb_msleep(1);
	}
}

// Check for button status
int getButton() {
	int id;
	if (gpioStat(P_SELBTN)) {
		id = P_SELBTN;
	} else if (gpioStat(P_MODEBTN)) {
		id = P_MODEBTN;
	} else {
		return 0;
	}

	// Wait until button is up
	while (gpioStat(id)) {
		amb_msleep(1);
	}

	return id;
}

// This interprets and runs a menu from the structure.
int runMenu(struct MenuItem menu[]) {
	int temp;
	
	drawGUI();
	drawMenu(menu);
	while (1) {
		int r = getButton();
		if (r == P_SELBTN) {
			if (menu[sel].type == ACTION) {
				drawGUI();
				temp = sel;
				sel = 0;
				line = 0;
				if (menu[temp].action(temp) != 0) {
					return 0;
			}
			} else if (menu[sel].type == SELECT) {
				while (1) {
					r = getButton();
					if (r == P_MODEBTN) {
						menu[sel].info->s++;
						if (menu[sel].info->elements[menu[sel].info->s] == 0) {
							menu[sel].info->s = 0;
						}

						drawGUI();
						drawMenu(menu);
					} else if (r == P_SELBTN) {
						break;
					}
					
					amb_msleep(1);
				}
			} else if (menu[sel].type == RETURN) {
				temp = sel;
				sel = 0;
				return temp;
			}
		} else if (r == P_MODEBTN) {
			if (menu[sel + 1].text == 0) {
				sel = 0;
			} else {
				sel++;
			}
		} else {
			continue;
		}

		drawGUI();
		drawMenu(menu);
		
		amb_msleep(10);
	}
}

#ifdef AMB_EXP
	struct ItemInfo selectISO = {
		0, {"200", "800", "1600", "3200", "6400", 0}
	};

	struct ItemInfo selectExp = {
		0, {"8", "7", "6", "5", "4", "3", "2", "1", 0}
	};

	char *shutterCode[] = {"1", "24", "85", "126", "178", "252", "378"};
	char *hijackExp[] = {"ia2", "-ae", "exp", 0, 0};

	int expTake() {
		hijackExp[3] = selectISO.elements[selectISO.s];
		hijackExp[4] = shutterCode[selectExp.s];
		amb_exp(envg, hijackExp);
		return 0;
	}

	struct MenuItem expMenu[] = {
		{"ISO", 0, SELECT, &selectISO},
		{"Exp", 0, SELECT, &selectExp},
		{"Apply", expTake, ACTION, 0},
		{"Exit", 0, RETURN, 0},
		{0}
	};
#endif

int expSetting() {
	runMenu(expMenu);
	return 0;
}

struct MenuItem mainMenu[] = {
	{"Exit", 0, RETURN, 0},
	{"About AHDK", ahdkInfo, ACTION, 0},
#ifdef AMB_EXP
	{"Manual", expSetting, ACTION, 0},
#endif
	{"Run Ambsh", showScripts, ACTION, 0},
	{"BrainF*ck", bfExec, ACTION, 0},
	{"CrypticOS", runCins, ACTION, 0},
	{0}
};

void start(int *env) {
	envg = env;
	line = 0;
	sel = 0;

	// Copy font into memory
	FILE *file = amb_fopen("d:/ahdk/font.bin", "r");
	if (!file) {amb_printf(env, "No font"); return;}
	amb_fread(font, FONTSIZE, FONTSIZE, file);
	amb_fclose(file);

	amb_printf(env, "Loaded font, %d bytes\n", FONTSIZE);

	runMenu(mainMenu);
}
