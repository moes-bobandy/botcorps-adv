#pragma once
#include <stdint.h>
namespace lgfx { inline namespace v1 {
class LovyanGFX {
public:
    template <class F> void setFont(const F *) {}
    void setTextSize(float) {}
    void setTextSize(float, float) {}
    void setTextColor(uint16_t) {}
    void setTextColor(uint8_t) {}
    void setCursor(int, int) {}
    void print(const char *) {}
    void fillRect(int, int, int, int, uint16_t) {}
    void fillScreen(uint16_t) {}
    void startWrite() {}
    void endWrite() {}
    void setClipRect(int, int, int, int) {}
    void clearClipRect() {}
    int width() const { return 240; }
    int height() const { return 135; }
};
class LGFX_Device : public LovyanGFX {};
class LGFX_Sprite : public LovyanGFX {
public:
    explicit LGFX_Sprite(LovyanGFX *) {}
    void setColorDepth(int) {}
    bool createSprite(int, int) { return false; }
    void *getBuffer() { return nullptr; }
    void pushSprite(int, int) {}
    void pushSprite(LovyanGFX *, int, int) {}
};
}}
