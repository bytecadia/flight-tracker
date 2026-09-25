#include "led-matrix.h"
#include "graphics.h"

#include "layout.hpp"
#include "image.hpp"
#include "canvas.hpp"
#include "render.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"

// TODO:: Move this out of render

void draw(const std::vector<Position> &positions, Canvas &canvas)
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

void render(std::stop_token st, Snapshot &snap, const Config &cfg, SQLite::Database &db)
{
#ifdef __APPLE__
// Not on pi
#else
    rgb_matrix::Options options;
    options.rows = cfg.rows;
    options.cols = cfg.cols;

    rgb_matrix::RuntimeOptions runtime_opt;

    RGBMatrix *mtrx = rgb_matrix::CreateFromOptions(options, runtime_opt);
    if (mtrx == NULL)
        return;

    Canvas canvas(mtrx->CreateFrameCanvas(), 0, cfg.cols);

    // TODO: add checks + logging for success and add font path
    rgb_matrix::Font font;
    if (!font.LoadFont(bdf_font_file))
    {
        fprintf(stderr, "Couldn't load font '%s'\n", bdf_font_file);
        return 1;
    }
    rgb_matrix::Font sml;
    rgb_matrix::Font med;
    rgb_matrix::Font lrg;
    sml.LoadFont((std::format("{}/{}", FONT_DIR, cfg.sml_fnt).c_str()));
    med.LoadFont((std::format("{}/{}", FONT_DIR, cfg.med_fnt).c_str()));
    lrg.LoadFont((std::format("{}/{}", FONT_DIR, cfg.lrg_fnt).c_str()));

    ImgCache cache;
    auto start = std::chrono::steady_clock::now();
    while (!st.stop_requested())
    {
        auto a = snap.read(); // TODO!: what did I mean by this -> (TODO: Make this blocking)
                              // ^ Ahh I see it checks and returns immediately if there
                              // is no aircraft to read and will keep looping
                              //  if (st.stop_requested())
                              //      return;
        // TODO: Create an idle screen?

        if (!a)
            continue;

        DisplayData data{db, a, cfg};
        DisplayLayout disp{data, sml, med, lrg, 0, 0, cfg.cols, cfg.rows, cfg.padding, cfg.row_gap, cfg.img_span};

        canvas.Clear();

        std::optional<Image> img = img_cache.get_image();

        if (img)
            draw_image(canvas, disp.content.lft(), disp.content.tp(), *logo);

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        auto positions = layout(disp, elapsed);

        draw(positions, canvas);
        canvas = mtrx->SwapOnVSync(canvas.GetRGBMatrix());
    }

    delete mtrx;
#endif
}