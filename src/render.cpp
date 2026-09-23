#include <optional>

#include "led-matrix.h"
#include "graphics.h"

#include "layout.hpp"
#include "snapshot.hpp"
#include "image.hpp"
#include "canvas.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

// TODO:: Move this out of render

void draw(const std::vector<Position> &positions, Canvas canvas)
{
    for (const auto &pos : positions)
    {
        canvas.SetClipBounds(pos.l, pos.r);
        rgb_matrix::DrawText(&canvas,
                             *pos.text.font,
                             pos.x, pos.y,
                             *pos.text.color,
                             nullptr, // TODO:: Do I want background colors?
                             pos.text.items.c_str(),
                             0); // TODO: Add named parameters
    }
}

void render(std::stop_token st, Snapshot &snap, Config &cfg)
{
#ifdef __APPLE__
// Not on pi
#else
    RGBMatrix::Options options;
    options.rows = cfg.rows;
    options.cols = cfg.cols;

    rgb_matrix::RuntimeOptions runtime_opt;

    RGBMatrix *mtrx = RGBMatrix::CreateFromOptions(options, runtime_opt);
    if (mtrx == NULL)
        return;

    Canvas canvas(mtrx->CreateFrameCanvas(), 0, cfg.cols);

    // TODO: add checks + logging for success and add font path
    rgb_matrix::Font sml = font.LoadFont((std::format("{}/{}", FONT_DIR, cfg.sml_font).c_str());
    rgb_matrix::Font med = font.LoadFont((std::format("{}/{}", FONT_DIR, cfg.med_font).c_str());
    rgb_matrix::Font lrg = font.LoadFont((std::format("{}/{}", FONT_DIR, cfg.lrg_font).c_str());

    auto start = std::chrono::steady_clock::now();
    while (!st.stop_requested())
    {
        auto a = snap.read(); // TODO: what did I mean by this -> (TODO: Make this blocking)
        if (st.stop_requested)
            return;
        DisplayLayout disp{a, sml, med, lrg};

        auto elapsed = std::chrono::duration_cast<std::chrono::millisecongs>(std::chrono::steady_clock::now() - start).count();
        auto positions = layout(disp, elapsed);

        canvas.Clear();

        if (a.has_logo)
        {
            Image logo = load_image(std::format("./assets/{}", a.airline));
            draw_image(canvas, disp.content.lft(), disp.content.tp(), logo);
        }
        draw(positions, canvas);
        canvas = mtrx->SwapOnVSync(canvas.GetRGBMatrix());
    }

    delete mtrx;
#endif
}