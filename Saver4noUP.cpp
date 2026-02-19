#include <fstream>
#include <iostream>
#include "Saver4noUP.h"
#include "Nearest.h"
#include "base.h"
#include "PaletteStatistics.h"

Saver4noUP::Saver4noUP(const uchar* avail_zx_palette_as_rgb)
    : Saver4(avail_zx_palette_as_rgb)
{
}

GlobalStat Saver4noUP::AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in)
{   
    auto nearest_low = Nearest{ _zx_palette,
        avail_paletteTC,
        {8, 9, 10, 11, 12, 13, 14, 15} };

    auto nearest_high = Nearest{ _zx_palette,
        avail_paletteTC,
        {0, 1, 2, 3, 4, 5, 6, 7} };

    PaletteStatistics ps_low{ unsigned(in.rows), (unsigned)in.cols,
                    in,
                    nearest_low, 0, 0 };

    auto& rev_col_map_low = ps_low.GetStat();
    std::cout << "GLOBAL:" << std::endl;
    auto it_s = rev_col_map_low.rbegin();
    std::optional<ColorStatInfo> most_popular_low;
    std::optional<ColorStatInfo> most_popular_high;

    for (unsigned k = 0;
        (k < 8) && it_s != rev_col_map_low.rend();
        ++k, ++it_s)
    {
        std::cout << "    count: " << it_s->first
            << ", r: " << (int)it_s->second.rgb.r
            << ", g: " << (int)it_s->second.rgb.g
            << ", b: " << (int)it_s->second.rgb.b
            << std::hex
            << ", COL: 0x" << (int)Pack(it_s->second.rgb)
            << std::dec
            << ", ZX color " << it_s->second.entry_indx << ", dist " << it_s->second.entry_distance
            << std::endl;

        if (!most_popular_low.has_value() && k == 0
            || !most_popular_high.has_value())
        {
            auto near_h = nearest_high.GetNearest(Expand(it_s->second.rgb));
            if (!most_popular_low.has_value() && it_s->second.entry_distance < near_h.distance)
            {
                most_popular_low = it_s->second;
            }
            else
            {
                auto stat = ColorStatInfo{ it_s->second.rgb, near_h.indx, near_h.distance };
                most_popular_high = stat;
                std::cout << "\t\t\t\t\t **** ZX color " << near_h.indx << ", dist " << near_h.distance
                    << std::endl;
            }
            std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
        }
        else if (!most_popular_low.has_value())
        {
            most_popular_low = it_s->second;
            std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
        }
    }

    if (most_popular_low.has_value())
    {
        _g.col_global0_indx = most_popular_low->entry_indx;
        _g.col_global0 = _zx_palette[*_g.col_global0_indx];
        _g.col_global0_rgb = most_popular_low->rgb;
    }
    if (most_popular_high.has_value())
    {
        _g.col_global1_indx = most_popular_high->entry_indx;
        _g.col_global1 = _zx_palette[*_g.col_global1_indx];
        _g.col_global1_rgb = most_popular_high->rgb;
    }
    return _g;

}
cv::Vec3b Saver4noUP::CodePixel(unsigned row, unsigned col,
    const cv::Vec3b& p,
    const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base)
{
    int dist_c0 = _g.col_global0_indx.has_value()
        ? Dist(this->_zx_palette[*_g.col_global0_indx], p)
        : std::numeric_limits<int>::max();
    int dist_c1 = _g.col_global1_indx.has_value()
        ? Dist(this->_zx_palette[*_g.col_global1_indx], p)
        : std::numeric_limits<int>::max();

    auto nearest_palette = NearestPal(pal_rgb, pal_indx_base, 2, p);

    cv::Vec3b best;
    unsigned code = 0;
    if (dist_c0 <= dist_c1 && dist_c0 < nearest_palette.distance) // prefer local attributes, as they have 16 not 8 colors
    {
        best = Expand(this->_zx_palette[*_g.col_global0_indx]);
        code = 0b00; // code for BACKGROUND color
    }
    else if (dist_c1 < nearest_palette.distance)
    {
        best = Expand(this->_zx_palette[*_g.col_global1_indx]); // *_g.col_global1);
        code = 0b10; // code for TIMEX color
    }
    else
    {
        best = Expand(pal_rgb[nearest_palette.indx + pal_indx_base]);
        code = (nearest_palette.indx << 1) | 0b01;
    }

    PutPixel(row, col, code);
    return best;

}

std::set<RGB> Saver4noUP::UsePrevPaletteEntries(const std::vector<RGB>& /*pal_rgb*/,
    unsigned /*pal_indx_base*/,
    unsigned /*current_column*/,
    unsigned /*current_row*/) const
{
    std::set<RGB> arleady_avail;
    if (_g.col_global0_rgb.has_value())
        arleady_avail.insert(*_g.col_global0_rgb);
    if (_g.col_global1_rgb.has_value())
        arleady_avail.insert(*_g.col_global1_rgb);
    return arleady_avail;
}
/*
void Saver4noUP::SaveHeader(std::ofstream& of, const std::string& project)
{

    _SaveHeader(of, fullname);
}


void Saver4noUP::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    std::string fullname = project + "4np";
    Saver4::SaveCFile(of, fullname, attribs);
}
*/

void Saver4noUP::_Save4thColor(std::ofstream& of, const std::string& project) const
{
}

void Saver4noUP::_Save0thColor(std::ofstream& of, const std::string& project) const
{
}

std::string Saver4noUP::GetFullProjectName(const std::string& core) const
{
    return core + "4np";
}
