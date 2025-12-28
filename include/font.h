#pragma once

#include <cstdint>

struct Font {
    int8_t y_adv;
    int8_t x_adv_max;
    int8_t x_off_min;
    int8_t x_off_max;
    int8_t y_off_min;
    int8_t y_off_max;
    struct {
        int32_t off;
        int8_t w;
        int8_t h;
        int8_t x_off;
        int8_t y_off;
        int8_t x_adv;
    } info[128];
    const uint8_t *data;

    constexpr bool printable(char c) const
    {
        // If 'char' is signed, we need c >= 0
        // If 'char' is unsigned, we need c <= 127
        // Either case means (c & 0x80) == 0
        return (c & 0x80) == 0;
    }

    constexpr int8_t height() const { return y_adv; }

    constexpr int8_t width(char c) const
    {
        return printable(c) ? info[int(c)].x_adv : 0;
    }

    constexpr int width(const char *s) const
    {
        int w = 0;
        while (*s != '\0') {
            char c = *s++;
            if (printable(c))
                w += info[int(c)].x_adv;
        }
        return w;
    }

    constexpr int8_t max_width() const { return x_adv_max; }
};
