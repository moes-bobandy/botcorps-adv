#pragma once
/* Cardputer Adv landscape dual front-end for the CYD livescan UI.
 * EXT 320x240 is the landscape plate. INT 240x135 is the status strip.
 * Built only for env cardputer-adv-dual (-DBOTCORPS_ADV_LAND=1). */
void lv_adv_begin(void);
void lv_adv_loop(void);
