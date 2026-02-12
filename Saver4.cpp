#include <fstream>
#include <iostream>
#include "Saver4.h"
#include "Nearest.h"
#include "base.h"
#include "PaletteStatistics.h"

Saver4::Saver4(const uchar* avail_zx_palette_as_rgb)
    : Saver(avail_zx_palette_as_rgb)
{
}

GlobalStat Saver4::AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in)
{   
    auto nearest_high = Nearest{ _zx_palette,
         avail_paletteTC,
         {0, 1, 2, 3, 4, 5, 6, 7} };


    PaletteStatistics ps{ unsigned(in.rows), (unsigned)in.cols,
                    in,
                    nearest_high, 0, 0 };
    auto nearest_low = Nearest{ _zx_palette,
        avail_paletteTC,
        {8, 9, 10, 11, 12, 13, 14, 15} };


    PaletteStatistics ps_low{ unsigned(in.rows), (unsigned)in.cols,
                    in,
                    nearest_low, 0, 0 };

    auto& rev_col_map_low = ps_low.GetStat();
    std::cout << "GLOBAL:" << std::endl;
    auto it_s = rev_col_map_low.rbegin();
    std::optional<ColorStatInfo> most_popular_low;
    for (unsigned k = 0;
        (k < 8) && it_s != rev_col_map_low.rend();
        ++k, ++it_s)
    {
        std::cout << "    count: " << it_s->first
            << ", r: " << (int)it_s->second.rgb.r
            << ", g: " << (int)it_s->second.rgb.g
            << ", b: " << (int)it_s->second.rgb.b
            << std::hex
            << ", COL: 0x" << ((it_s->second.rgb.g << 5) + (it_s->second.rgb.r << 2) + it_s->second.rgb.b)
            << std::dec
            << ", ZX color " << it_s->second.entry_indx << ", dist " << it_s->second.entry_distance
            << std::endl;
        if (!most_popular_low.has_value())
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

    // now lets cut the rows for "extended timex color"
    auto nearest = Nearest{ _zx_palette,
        avail_paletteTC,
        {} };
    
    _color0.resize(24);
    _color4.resize(24);
    for (unsigned row = 0; row < 192; row+=8)
    {
        PaletteStatistics ps{ 8, (unsigned)in.cols,
                        in,
                        nearest, row, 0 };

        auto& rev_col_map = ps.GetStat();
        bool stored0{ false };
        bool stored4{ false };
        std::cout << "Analyzing row " << row << std::endl;
        for (auto it_s = rev_col_map.rbegin(); it_s != rev_col_map.rend(); ++it_s)
        {
            std::cout << "    count: " << it_s->first
            << ", r: " << (int)it_s->second.rgb.r
            << ", g: " << (int)it_s->second.rgb.g
            << ", b: " << (int)it_s->second.rgb.b
            << std::hex
            << ", COL: 0x" << ((it_s->second.rgb.g << 5) + (it_s->second.rgb.r << 2) + it_s->second.rgb.b)
            << std::dec
            << ", ZX color " << it_s->second.entry_indx << ", dist " << it_s->second.entry_distance
            << std::endl;

            if (!stored0)
            {
                _color0[row >> 3] = it_s->second.rgb;
                stored0 = true;
            }   
            else if (!stored4)
            {
                _color4[row >> 3] = it_s->second.rgb;
                stored4 = true;
            }
        }
        if (!stored0)
            _color0[row >> 3] = RGB{};
        if (!stored4)
            _color4[row >> 3] = RGB{};
    }

    return _g;
}

void Saver4::PutPixel(unsigned row, unsigned col, unsigned val)
{
    uint8_t triple_mask = row & 0xC0;
    uint8_t line_in_char = row & 0x07;
    uint8_t line_in_block = (row & 0x38) >> 3;
    uint16_t addr_by_row = (triple_mask + (line_in_char << 3) + line_in_block) << 5;
    uint8_t col_shift = (col >> 3) & 0x1F;
    uint16_t addr = addr_by_row + col_shift;
    // 0 1 2 3 in byte
    unsigned char mask = 0;
    unsigned bits2 = 0;
    switch (col & 0x03)
    {
    case 0x00:
        mask = 0b00111111;
        bits2 = val << 6;
        break;
    case 0x01:
        mask = 0b11001111;
        bits2 = val << 4;
        break;
    case 0x02:
        mask = 0b11110011;
        bits2 = val << 2;
        break;
    case 0x03:
        mask = 0b11111100;
        bits2 = val;
        break;
    }
    // pixels 0-3 goes to first bank, 4-7 to second
    if ((col & 0x04) == 0)
    {
        out_page0[addr] = (out_page0[addr] & mask) | bits2;
    }
    else
    {
        out_page1[addr] = (out_page1[addr] & mask) | bits2;
    }
}

cv::Vec3b Saver4::CodePixel(unsigned row, unsigned col,
    const cv::Vec3b& p,
    const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base)
{
/*    int dist_c0 = _g.col_global0_indx.has_value()
        ? Dist(this->_zx_palette[*_g.col_global0_indx], p)
        : std::numeric_limits<int>::max();
        */
    unsigned ext_color_index = row >> 3;
    const auto& color0 = _color0[ext_color_index];
    // no use of extended colors in first 8 rows
    int dist_c0 = ext_color_index > 0 ? Dist(color0, p) : std::numeric_limits<int>::max();

    // 4th color
    const auto& color4 = _color4[row >> 3];
    int dist_c1 = ext_color_index > 0 ? Dist(color4, p) : std::numeric_limits<int>::max();
 
    auto nearest_palette = NearestPal(pal_rgb, pal_indx_base, 2, p);

    cv::Vec3b best;
    unsigned code = 0;
    if (dist_c0 <= dist_c1 && dist_c0 < nearest_palette.dinstance) // prefer local attributes, as they have 16 not 8 colors
    {
        best = Expand(color0);
        code = 0b00; // code for BACKGROUND color
    }
    else if (dist_c1 < nearest_palette.dinstance)
    {
        best = Expand(color4); // *_g.col_global1);
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

std::set<RGB> Saver4::UsePrevPaletteEntries(const std::vector<RGB>& /*pal_rgb*/,
    unsigned /*pal_indx_base*/,
    unsigned /*current_column*/,
    unsigned current_row) const
{
    std::set<RGB> arleady_avail;
    // 4th color
    unsigned ext_color_index = current_row >> 3;
    if (ext_color_index > 0)
    {
        const auto& color4 = _color4[ext_color_index];
        arleady_avail.insert(color4);
        const auto& color0 = _color0[ext_color_index];
        arleady_avail.insert(color0);
    }
    //if (_g.col_global1_rgb.has_value())
    //    arleady_avail.insert(*_g.col_global1_rgb);
    return arleady_avail;
}

void Saver4::_SaveHeader(std::ofstream& of, const std::string& project) const
{
    of << "#define " << project << "col0 0x" << std::hex << *_g.col_global0_indx << std::endl;
    of << "#define " << project << "rgb0 0x" << std::hex << (int)Pack(*_g.col_global0_rgb) << std::endl;

    of << "extern void " << project << "_show() __banked;" << std::endl;
    of << "extern void " << project << "_prepare_colors() __banked;" << std::endl;
}

void Saver4::SaveHeader(std::ofstream& of, const std::string& project)
{
    std::string fullname = project + "4";
    _SaveHeader(of, fullname);
}

void Saver4::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    std::string fullname = project + "4";
    Saver::SaveCFile(of, fullname, attribs);
    of << "void SetColor(unsigned char entry, unsigned char value);" << std::endl;
    of << "void " << fullname << "_prepare_colors()" << std::endl;
    of << "{" << std::endl;
    _Save4thColor(of, fullname);
    _Save0thColor(of, fullname);
    of << "}" << std::endl;
}

void Saver4::_Save4thColor(std::ofstream& of, const std::string& project) const
{
    for (unsigned clutch = 0; clutch < 3; clutch++)
    {
        for (unsigned c = 0; c < 8; c++)
        {
            unsigned uplus_index = (clutch << 4) + c;
            unsigned index = (clutch << 3) + c + 1;
            if (index < 24)
                of << "    SetColor(" << std::dec << uplus_index << ", 0x"
                    << std::hex << (int)Pack(_color4[index]) << ");" << std::endl;
        }
    }
}

void Saver4::_Save0thColor(std::ofstream& of, const std::string& project) const
{
    for (unsigned clutch = 0; clutch < 3; clutch++)
    {
        for (unsigned c = 0; c < 8; c++)
        {
            unsigned uplus_index = (clutch << 4) + c + 8;

            unsigned index = (clutch << 3) + c + 1;
            if (index < 24)
                of << "    SetColor(" << std::dec << uplus_index << ", 0x"
                << std::hex << (int)Pack(_color0[index]) << ");" << std::endl;
        }
    }
}
