# v1.2.12: SIM-ONLY, not hardware-tested, not flashed
## GIBSON scan screen: hunt meter removed (Mobandy 10:44 AM ET, pt1477u)
- The top hunt bar's 11-bar meter (x84-148, y6-20) used to draw over the target name. It is now off. HUNT, MAC, name, dBm and STEADY/WARMER/COLDER are unchanged.
- The LOCKED A/V2 bands in the hunted tower's glass are off too. The live level still shows in the HUNT folder bar.
- Neither is deleted: the code is kept behind LV_RD_HM_TOPBAR / LV_RD_HM_TOWER (both 0).
- Gate: hunt frames differ only at x84-148 / y6-20 (the meter). Everything else is 0 px.
## CONTROLbot panel (Mobandy 11:03 AM ET)
- RECAL TOUCH (CHIME's spot) runs CALbot. It saves c0..c7, then cv last, through r_cal_save. A timeout keeps the old cal.
- ROTATE (TIME's spot) toggles PORT / FLIP (setRotation 2/0), saved as NVS lkrot. LovyanGFX rotates the touch with the panel.
- EXIT (TONE's spot) shows LNCHR when a Launcher app is in the `test` partition (Launcher 3.0 custom_4Mb.csv), otherwise OFF.
  - With a Launcher: it tries esp_ota_set_boot_partition, then restarts. Stock IDF rejects the test subtype, so it falls back to a 200 ms deep sleep. Launcher 3.0's bootloader opens the Launcher on a deep-sleep wake (unless its DDLB option is on).
  - Without a Launcher: backlight and panel off, then deep sleep. The touch IRQ (GPIO36, low) wakes it. It first waits for the finger to lift.
  - Nothing is erased.
- CHIME / TIME / TONE values stay in NVS and in code (the alert chime still uses the saved value).
- SCAN is now labelled RADAR RAIN / SWEEP / OFF. See the findings below.
## SCAN findings
1. Tap area, cycling, NVS save (cbscan) and boot load were all working in the sim. Every tap cycled B.RAIN, S.RAIN, OFF and saved.
2. It never touched WiFi/BLE scanning. It only changes the GIBSON radar background: B.RAIN = code rain plus drifting camera, S.RAIN = code rain plus sweep line, OFF = no code rain. So nothing visible changes on the CONTROLbot page or on any pager page. The label said SCAN, so it looked broken.
3. The bar's touch target is 68x22 px, below the 36-40 px floor. It sits 2 px above SLEEP.
4. Side effect: SWEEP also stops the Dojo idle sway (the Dojo reads the same lv_rainmode).
