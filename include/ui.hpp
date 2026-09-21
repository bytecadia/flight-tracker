#pragma once

#include <string>
#include <vector>
#include "graphics.h"

// TODO: These are for image logic only
#include <optional>
#include <spdlog/spdlog.h>
#include "stb_image.h"
#include "stb_image_resize2.h"

struct Text
{
    const std::string items;
    const rgb_matrix::Font &font;
    const rgb_matrix::Color &color;
};

enum class Mode
{
    Clip,
    Fit,
    Scroll
};

struct Element
{
    Mode mode;
    int space;
    std::vector<Text> items;
};

struct Row
{
    Mode mode;
    std::vector<Element> items;
    int h;
    int gap;
};

struct Position
{
    Text text;
    int x;
    int y;
    int l;
    int r;
};

class Rect
{
public:
    int x;
    int y;
    int h;
    int w;

    Rect(int x, int y, int h, int w) : x(x), y(y), h(h), w(w) {}

    void inset(int padding)
    {
        x += padding;
        y += padding;
        h -= padding * 2;
        w -= padding * 2;
    }

    int lft() { return x; }
    int rght() { return x + w; }
    int tp() { return y; }
    int btm() { return y - h; }
};

// TODO: Probably won't include below here

struct Image
{
    int w = 0;
    int h = 0;
    std::vector<unsigned char> pixels;
};

std::optional<Image>
load_image(const std::string &path, int target_h)
{
    int w, h, channels;
    unsigned char *decoded = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (!decoded)
    {
        spdlog::error("Unable to load image at path '{}'", path);
        return std::nullopt;
    }
    int target_w = w * target_h / w; // Calculate new width
    Image img;
    img.w = target_w;
    img.h = target_h;
    img.pixels.resize(target_w * target_h * 4);

    stbir_resize_uint8_srgb(decoded, w, h, 0, img.pixels.data(), target_w, target_h, 0, STBIR_RGBA);

    stbi_image_free(decoded);
    return img;
}

void draw_image(rgb_matrix::Canvas *c, int x, int y, Image img)
{
    for (size_t iy = 0; iy < img.h; ++iy)
    {
        for (size_t ix = 0; ix < img.w; ++ix)
        {
            const unsigned char *px = &img.pixels[(y * img.w + x) * 4];
            if (px[3] > 0)
                c->SetPixel(x + ix, y + iy, px[0], px[1], px[2]);
        }
    }
}