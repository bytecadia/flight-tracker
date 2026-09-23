#include <optional>

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
    int target_w = w * target_h / h; // Calculate new width
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
            const unsigned char *px = &img.pixels[(iy * img.w + ix) * 4];
            if (px[3] > 0)
                c->SetPixel(x + ix, y + iy, px[0], px[1], px[2]);
        }
    }
}

// Skip drawing pixels outside of clip bounds
int Font::DrawGlyph(Canvas *c, int x_pos, int y_pos,
                    const Color &color, const Color *bgcolor,
                    uint32_t unicode_codepoint,
                    int l_clip, int r_clip) const
{
    const Glyph *g = FindGlyph(unicode_codepoint);
    if (g == NULL)
        g = FindGlyph(kUnicodeReplacementCodepoint);
    if (g == NULL)
        return 0;
    y_pos = y_pos - g->height - g->y_offset;

    if (x_pos + g->device_width < 0 || x_pos > c->width() ||
        y_pos + g->height < 0 || y_pos > c->height() ||
        x_pos + g->device_width <= l_clip || x_pos >= r_clip)
    {
        return g->device_width;
    }

    for (int y = 0; y < g->height; ++y)
    {
        const rowbitmap_t &row = g->bitmap[y];
        for (int x = 0; x < g->device_width; ++x)
        {
            const int sx = x_pos + x;
            if (sx < l_clip || sx >= r_clip)
                continue;
            if (row.test(kMaxFontWidth - 1 - x))
            {
                c->SetPixel(sx, y_pos + y, color.r, color.g, color.b);
            }
            else if (bgcolor)
            {
                c->SetPixel(sx, y_pos + y, bgcolor->r, bgcolor->g, bgcolor->b);
            }
        }
    }
    return g->device_width;
}

// Pass down clip parameters
int DrawText(Canvas *c, const Font &font,
             int x, int y, const Color &color, const Color *background_color,
             const char *utf8_text, int extra_spacing,
             int l_clip = INT_MIN, int r_clip = INT_MAX)
{
    const int start_x = x;
    while (*utf8_text)
    {
        const uint32_t cp = utf8_next_codepoint(utf8_text);
        x += font.DrawGlyph(c, x, y, color, background_color, cp, l_clip, r_clip);
        x += extra_spacing;
    }
    return x - start_x;
}

void render(std::stop_token st, Snapshot &snap)
{
    // TODO: Config here
    RGBMatrix::Options options;
    options.rows = 32;
    options.cols = 64;

    rgb_matrix::RuntimeOptions runtime_opt;

    RGBMatrix *mtrx = RGBMatrix::CreateFromOptions(options, runtime_opt);
    if (mtrx == NULL)
        return;

    FrameCanvas *canvas = mtrx->CreateFrameCanvas();

    // TODO: add checks for success and add font path
    // TODO: config or just normal path here hzeller has these (.bdf)s
    rgb_matrix::Font sml = font.LoadFont("");
    rgb_matrix::Font med = font.LoadFont("");
    rgb_matrix::Font lrg = font.LoadFont("");

    auto start = std::chrono::steady_clock::now();
    while (!st.stop_requested())
    {
        ca auto a = snap.read(); // TODO: Make this blocking
        if (st.stop_requested)
            return;
        Theme t = (a.op == Aircraft::Operations::Com) ? Theme::Comm : Theme::Non;
        AircraftDisplay disp{
            a,
            t,
            sml,
            med,
            lrg,
        };

        auto elapsed = std::chrono::duration_cast<std::chrono::millisecongs>(std::chrono::steady_clock::now() - start).count();
        auto positions = layout(disp, elapsed);

        canvas->Clear();

        if (a.has_logo)
        {
            Magick::Image logo = load_image(std::format("./assets/{}", a.airline));
            draw_image(canvas, disp.content.lft(), disp.content.tp(), logo);
        }
        draw(positions, canvas);
        canvas = mtrx->SwapOnVSync(canvas);
    }

    delete mtrx;
}