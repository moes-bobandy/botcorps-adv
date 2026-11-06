# Field checklist: port 1105 (b29f8ab) on the CYD

Open GIBSON radar (needs a few finds; let a scan finish).

1. Tap empty space: selection cycles strongest -> weakest dBm and wraps; it never jumps on its own.
2. HOLD TO LOCK bar (lower right of the panel): fills 0 -> 100 % over 1 s; the ice-blue D4 current runs along the grid lines
   from the panel to the tower (calm, breathing, packets every 40 px). Let go early: it drains. Hold to full: lock fires.
3. Locked: tower lit in its own colour, IGNORE / WATCH / HUNT row, current keeps flowing into the tower base.
4. Tap HUNT. The top banner turns into the hunt bar: HUNT ..MAC, name, big dBm, STEADY / WARMER / COLDER / READ n all still
   there; the 11 rising bars are GONE (a small dim tower glyph sits in that slot).
5. Inside the hunted tower's glass: 14 thermometer bands, dim at first, then lighting one by one from the base up as packets
   arrive: green at the base -> yellow -> red at the top. Target fill follows real dBm: about -86 dBm 3 bands, -66 dBm 8,
   -40 dBm and stronger all 14. Move closer / farther: it climbs / drains one band per packet.
6. At the tower base (where the current meets the glass): a soft breathing ice contact glow; each packet (~every 0.2 s)
   throws a short stepped ice arc into one of the lowest green bands, and a brightness ripple climbs the lit bands.
   Nothing is brighter than the banner's dBm / yellow text.
7. Camera drift: the bands stay inside the tower as it slides.
8. STOP HUNT: bands disappear, LOCK label back, the banner returns to the scrolling ticker. HUNT again: meter restarts empty.
9. Tap elsewhere: lock releases, current stops. BACKbot returns home. Home / SCANbot / TERMINAL look unchanged.
10. Watch for: frame-rate drop while hunting, bands lagging the tower, any flicker at the contact; report what you see.
