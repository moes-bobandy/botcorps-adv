#pragma once

/*
 * Mock screens on the Cardputer Adv.
 * EXT (320x240) is the main UI: icon buttons and the screen.
 * INT (240x135) is the selected bot's ID card. It has no menu.
 */

void botcorps_ui_mark_dirty(void);
void botcorps_ui_begin(void);
void botcorps_ui_input(void);
bool botcorps_ui_wants_ext_rain(void);
void botcorps_ui_draw(void);
