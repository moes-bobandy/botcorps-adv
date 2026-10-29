// cyd-sim: CYDpaint font atlas harness (sim-only; firmware untouched).
// Renders printable ASCII with LovyanGFX's own font code into a 16-bit sprite and dumps per-glyph
// bitmaps + advances as JSON, so CYDpaint draws exactly what setFont()/drawString() draw on the CYD.
//   usage: cydfont OUT.json
#include <LovyanGFX.hpp>
#include <cstdio>
#include <string>
struct F { const char* id; const char* name; const lgfx::IFont* f; bool fw; };
int main(int argc, char** argv) {
  if (argc < 2) { fprintf(stderr, "usage: %s OUT.json\n", argv[0]); return 2; }
  F fonts[] = { { "Font0", "Font0 (GLCD 6x8)", &fonts::Font0, true }, { "Font8x8C64", "Font8x8C64 (C64 8x8)", &fonts::Font8x8C64, true },
                { "Font2", "Font2 (16 px)", &fonts::Font2, false }, { "Font4", "Font4 (26 px)", &fonts::Font4, false } };
  FILE* o = fopen(argv[1], "w"); if (!o) return 1;
  lgfx::LGFX_Sprite sp; sp.setColorDepth(16); if (!sp.createSprite(96, 64)) return 1;
  fprintf(o, "{\"source\":\"LovyanGFX %s via cyd-sim sim/font_harness.cpp\",\"fonts\":[", "v1");
  for (size_t k = 0; k < sizeof(fonts) / sizeof(fonts[0]); ++k) {
    sp.setFont(fonts[k].f); sp.setTextSize(1); sp.setTextDatum(lgfx::textdatum::top_left);
    int fh = sp.fontHeight();
    fprintf(o, "%s{\"id\":\"%s\",\"name\":\"%s\",\"fw\":%s,\"h\":%d,\"g\":{", k ? "," : "", fonts[k].id, fonts[k].name, fonts[k].fw ? "true" : "false", fh);
    for (int c = 32; c < 127; ++c) {
      char s[2] = { (char)c, 0 }; sp.fillScreen(0); sp.setTextColor(0xFFFF); sp.drawString(s, 0, 0);
      int adv = sp.textWidth(s), W = 0;
      for (int y = 0; y < 64; ++y) for (int x = 0; x < 96; ++x) if (sp.readPixel(x, y) && x + 1 > W) W = x + 1;
      fprintf(o, "%s\"%d\":[%d,%d,\"", c > 32 ? "," : "", c, adv, W);
      for (int y = 0; y < fh; ++y) for (int x = 0; x < W; ++x) fputc(sp.readPixel(x, y) ? '1' : '0', o);
      fprintf(o, "\"]");
    }
    // string check: "Ag 0:/" width at size 1 and 2 = sum of advances?
    sp.setTextSize(2); int w2 = sp.textWidth("Ag 0:/"); sp.setTextSize(1); int w1 = sp.textWidth("Ag 0:/");
    fprintf(o, "},\"chk\":[%d,%d]}", w1, w2);
  }
  fprintf(o, "]}\n"); fclose(o); printf("wrote %s\n", argv[1]); return 0;
}
