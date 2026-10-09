#pragma once

/*
 * Mock screens on the Cardputer Adv.
 * EXT (320x240) is the main UI. INT (240x135) is the menu.
 * The name bar is painted here, on botcorps_menu() only.
 */

void botcorps_ui_begin(void);
void botcorps_ui_input(void);
bool botcorps_ui_wants_ext_rain(void);
void botcorps_ui_draw(bool ext_presented);
