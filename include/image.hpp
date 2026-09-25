#include <optional>

#include "ui.hpp"
#include "canvas.hpp"
#include "stb_image.h"
#include "stb_image_resize2.h"

std::optional<Image>
load_image(const std::string &path, int target_h)
{
    int w, h, channels;
    unsigned char *decoded = stbi_load(path.c_str(), &w, &h, &channels, 4);

    if (!decoded)
        return std::nullopt;

    int target_w = w * target_h / h; // Calculate new width
    Image img;
    img.w = target_w;
    img.h = target_h;
    img.pixels.resize(target_w * target_h * 4);

    stbir_resize_uint8_srgb(decoded, w, h, 0, img.pixels.data(), target_w, target_h, 0, STBIR_RGBA);

    stbi_image_free(decoded);
    return img;
}

void draw_image(Canvas &c, int x, int y, const Image &img)
{
    for (size_t iy = 0; iy < img.h; ++iy)
    {
        for (size_t ix = 0; ix < img.w; ++ix)
        {
            const unsigned char *px = &img.pixels[(iy * img.w + ix) * 4];
            if (px[3] > 0)
                c.SetPixel(x + ix, y + iy, px[0], px[1], px[2]);
        }
    }
}

class ImgCache
{
    std::unordered_map<std::string, std::optional<Image>> cache;

public:
    std::optional<Image> get_image(const std::string &path, int target_h)
    {
        auto it = cache.find(path);

        if (it == cache.end())
        {
            auto img = load_image(path, target_h);
            cache[path] = img;
            return img;
        }
        return it->second;
    }
};