#pragma once

#include "led-matrix.h"

class Canvas : public rgb_matrix::Canvas
{
    rgb_matrix::FrameCanvas *c;
    int l, r;

public:
    Canvas(rgb_matrix::FrameCanvas *c, int l, int r) : c(c), l(l), r(r) {}

    int width() const override { return c->width(); }
    int height() const override { return c->height(); }
    void Clear() override { c->Clear(); }
    void Fill(uint8_t r, uint8_t g, uint8_t b) override { c->Fill(r, g, b); }

    void SetPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) override
    {
        if (x >= this->l && x < this->r)
            c->SetPixel(x, y, r, g, b);
    }

    void SetClipBounds(int l, int r)
    {
        this->l = l;
        this->r = r;
    }

    rgb_matrix::Canvas *GetRGBMatrix() { return c; } // TODO: Do I even need this?
};
