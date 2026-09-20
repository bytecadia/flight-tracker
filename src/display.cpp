#include <string>
#include <SQLiteCpp/SQLiteCpp.h>

#include "ui.hpp"
#include "config.hpp"
#include "data.hpp"
#include "geometry.hpp"
#include "aircraft.hpp"
#include "colors.hpp"
#include "strings.hpp"

inline std::string to_str(AircraftType type)
{
    switch (type)
    {
    case AircraftType::Prop:
        return "Prop";
        break;
    case AircraftType::Jet:
        return "Jet";
        break;
    case AircraftType::Heli:
        return "Heli";
        break;
    case AircraftType::Unk:
        return "Unk";
        break;

    default:
        break;
    }
}

struct DisplayData
{
    std::string header; // Airline or manufacturer - this is why database is needed
    std::string img_path;
    std::string callsign;
    int alt;
    int speed;
    int distance;
    int bearing;
    int track;

    // TODO: Need to add check to process.cpp to ensure below is true
    DisplayData(SQLite::Database &db, const Aircraft &a,
                const Config &cfg) // Assumes aircraft has all info
    {
        std::string airline = lookup_airline(db, a.callsign);
        AircraftInfo info = lookup_aircraft(db, a.icao);

        img_path = std::format("assets/sprites/{}", to_str(info.type));

        if (!airline.empty())
        {
            header = airline;
            auto try_path = std::format("assets/airlines/{}", airline); // Hardcoded for now
            if (std::filesystem::exists(try_path))
                img_path = try_path;
        }
        else
            header = std::format("{} {}", info.mfc, info.mdl);

        callsign = a.callsign;
        alt = *a.alt;
        speed = *a.gs;
        distance = static_cast<int>(calc_dist(cfg.lat, cfg.lon, *a.lat, *a.lon));
        track = *a.trk;
        bearing = static_cast<int>(calc_bearing(cfg.lat, cfg.lon, *a.lat, *a.lon));
    }
};

// TODO: Everything below this is not compiling, everything above is

struct DisplayLayout
{
    std::vector<Row> rows;
    Rect content;
    int row_gap;

    DisplayLayout(const DisplayData &data,
                  const Font sml,
                  const Font med, const Font lrg, int x, int y, int w, int h, int padding, int row_gap) : content(x, y, w, h), row_gap(row_gap)
    {
        rows.push_back(Row{Mode::Scroll{Element{Mode::Scroll, 0, {Text{data.header, lrg, RED}}}}, 0});
        rows.push_back(Row{Mode::Fit{Element{Mode::Scroll, 0, {Text{data.callsign, lrg, RED}}}}, 0});
        rows.push_back(Row{Mode::Fit{Element{Mode::Fit, 2, {Text{std::to_string(data.speed), sml, BLUE}, Text{"mph", sml, LIGHT_BLUE}}},
                                     Element{Mode::Fit, 2, {Text{std::to_string(data.alt), sml, BLUE}, Text{"ft", sml, LIGHT_BLUE}}}},
                           2});

        rows.push_back(Row{Mode::Fit{Element{Mode::Fit, 2, {Text{std::to_string(data.distance), med, YELLOW}, Text{"mi", sml, LIGHT_YELLOW}}},
                                     Element{Mode::Fit, 0, {Text{std::to_string(data.bearing), med, YELLOW}}},
                                     Element{Mode::Fit, 2, {Text{"-", med, YELLOW}, Text{std::to_string(data.track), med, YELLOW}, Text{"°", sml, LIGHT_YELLOW}}}},
                           2});

        content.inset(padding);
    }
};

std::vector<Position> layout(const DisplayLayout &disp, int64_t time, Image img)
{
    const int y1 = disp.img.rows(); // Anchor to bottom line of logo
    const int y2 = y1 + disp.rows[2].h + disp.row_gap;
    const int y4 = disp.content.btm(); // Anchor to bottom of frame
    const int y3 = y4 + disp.rows[4].h + disp.row_gap;

    int x = disp.frame.left();
    const int x1 = (1 <= disp.logo_span) ? x + img.h : x;
    const int x2 = (2 <= disp.logo_span) ? x + img.h : x;
    const int x3 = (3 <= disp.logo_span) ? x + img.h : x;
    const int x4 = (4 <= disp.logo_span) ? x + img.h : x;

    std::vector<Position> pos;
    pos.push_back(lay_row(disp.rows[1], x1, y1));
    pos.push_back(lay_row(disp.rows[2], x2, y2));
    pos.push_back(lay_row(disp.rows[3], x3, y3));
    pos.push_back(lay_row(disp.rows[4], x4, y4));

    return pos;
}

std::vector<Position> lay_row(int x, int y, int r,
                              const Row &row,
                              std::optional<int64_t> time)
{
    std::vector<Position> pos;
    int start = x;

    for (size_t i = 0; i < row.items.size(); i++)
    {
        int w;
        Element elmnt = row.items[i];
        std::vector<Position> np = lay_elmnt(x, y, &w, row.gap, elmnt);

        if (x + w > r)
        {
            switch (row.mode)
            {
            case Mode::Fit:
                return pos;

            case Mode::Clip:
                pos.push_back(np);
                return pos;

            case Mode::scroll:
                std::vector<Element> rest(row.items.begin() + i, row.items.end());
                pos.push_back(np);
                x = scroll_placement(start, x, Row{rest, row.gap, row.mode});
                pos.push_back(lay_elmnt(x, y, &w, row.gap, elmnt));
                x += w;
                continue;
            }
        }

        pos.push_back(np);
        x += w;
    }
}

int scrl_plcmnt(int strt, int l, int r,
                int scrl_gap, int pps,
                int64_t time,
                const Row &rest)
{

    int scrl_ofst = time * (pps / 1000) % (r - l);
    strt += scrl_ofst;
    int w = msr_row(rest);
    int r = strt + w - r;
    int offset = w - leftover;
    int max_x = start - w - scrl_gap;

    if (l < max_x)
        return l - offset;

    return max_x - offset;
}

int msr(const std::string &text)
{
    return MeasureText(text.c_str());
}

template <typename T>
int msr(const T &t)
{
    int w = 0;
    const size_t n = t.items.size();
    for (size_t i = 0; i < n; i++)
        w += msr(t.items[i]);
    if (i + 1 < t.items.size())
        w += t.gap;
}
return w;
}

std::vector<Position> lay_elmnt(int x, int y, int &w, int gap,
                                Element &elmnt)
{
    int start = x;
    std::vector<Position> pos;
    for (size_t i = 0; i < elmnt.items.size(); i++)
    {
        Text itm = elmnt.items[i];
        Position np{itm, x, y};

        x += MeasureText(itm.value, itm.font);
        if (i < elmnt.items.size() - 1)
            x += gap;

        pos.push_back(np);
    }
    w = x - start;
}