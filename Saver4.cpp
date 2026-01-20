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


    PaletteStatistics ps{ unsigned(in.cols), (unsigned)in.rows,
                    in,
                    nearest_high, 0, 0 };
    auto nearest_low = Nearest{ _zx_palette,
        avail_paletteTC,
        {8, 9, 10, 11, 12, 13, 14, 15} };


    PaletteStatistics ps_low{ unsigned(in.cols), (unsigned)in.rows,
                    in,
                    nearest_low, 0, 0 };

    auto& rev_col_map_low = ps_low.GetStat();
    std::cout << "LOW" << std::endl;
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

    PaletteStatistics ps_high{ unsigned(in.cols), (unsigned)in.rows,
                     in,
                     nearest_high, 0, 0 };

    auto& rev_col_map_high = ps_high.GetStat();

    it_s = rev_col_map_high.rbegin();
    std::optional<ColorStatInfo> most_popular_high;
    std::cout << "HIGH" << std::endl;
    for (unsigned k = 0;
        (k < 8) && it_s != rev_col_map_high.rend();
        ++k, ++it_s)
    {
        std::cout << "    count: " << it_s->first
            << ", r: " << (int)(it_s->second.rgb.r)
            << ", g: " << (int)(it_s->second.rgb.g)
            << ", b: " << (int)(it_s->second.rgb.b)
            << std::hex
            << ", COL: 0x" << ((it_s->second.rgb.g << 5) + (it_s->second.rgb.r << 2) + it_s->second.rgb.b)
            << std::dec
            << ", ZX color " << it_s->second.entry_indx << ", dist " << it_s->second.entry_distance
            << std::endl;
        if (!most_popular_high.has_value())
        {
            bool a = it_s->second.rgb < most_popular_low->rgb;
            bool b = most_popular_low->rgb < it_s->second.rgb;
            if (!most_popular_low.has_value() || ((it_s->second.rgb < most_popular_low->rgb) || (most_popular_low->rgb < it_s->second.rgb)))
            {
                most_popular_high = it_s->second;
                std::cout << "^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^" << std::endl;
            }
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

cv::Vec3b Saver4::CodePixel(unsigned row, unsigned col, const cv::Vec3b& p, const std::vector<RGB>& pal_rgb, unsigned pal_indx_base)
{
    int dist_c0 = _g.col_global0_rgb.has_value()
        ? Dist(*_g.col_global0_rgb, p)
        : std::numeric_limits<int>::max();
    int dist_c1 = _g.col_global1_rgb.has_value()
        ? Dist(*_g.col_global1_rgb, p)
        : std::numeric_limits<int>::max();
    auto nearest_palette = NearestPal(pal_rgb, pal_indx_base, 2, p);

    cv::Vec3b best;
    unsigned code = 0;
    if (dist_c0 <= dist_c1 && dist_c0 < nearest_palette.dinstance) // prefer local attributes, as they have 16 not 8 colors
    {
        best = Expand(*_g.col_global0_rgb);
        code = 0b01; // code for BACkGROUND color
        //second_plane = 0b00; // ignored zero
    }
    else if (dist_c1 < nearest_palette.dinstance)
    {
        best = Expand(*_g.col_global1_rgb);
        code = 0b11; // code for TIMEX color
    }
    else
    {
        best = Expand(pal_rgb[nearest_palette.indx + pal_indx_base]);
        code = nearest_palette.indx << 1;
    }

    PutPixel(row, col, code);
    return best;

}

std::set<RGB> Saver4::UsePrevPaletteEntries(const std::vector<RGB>& /*pal_rgb*/,
    unsigned /*pal_indx_base*/,
    unsigned /*current_column*/) const
{
    std::set<RGB> arleady_avail;
    if (_g.col_global0_rgb.has_value())
        arleady_avail.insert(*_g.col_global0_rgb);
    if (_g.col_global1_rgb.has_value())
        arleady_avail.insert(*_g.col_global1_rgb);
    return arleady_avail;
}

void Saver4::SaveHeader(std::ofstream& of, const std::string& project)
{
    of << "#define " << project << "col0 0x" << *_g.col_global0_indx << std::endl;
    of << "#define " << project << "rgb0 0x" << (int)Pack(*_g.col_global0_rgb) << std::endl;
    of << "#define " << project << "col3 0x" << *_g.col_global1_indx << std::endl;

    of << "extern void " << project << "_show() __banked;" << std::endl;
}

void Saver4::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    std::string fullname = project + "4";
    Saver::SaveCFile(of, fullname, attribs);
}

/*
void Saver4::SavePalette(std::ofstream& of, const std::vector<RGB> pal, const std::string& prefix, const RGB* others) const
{
    unsigned zx_pal[48];
    for (unsigned i = 0; i < 48; i++)
    {
        // addressing bits are swapped, bit 3 swaps ink / paper
        // 00ccPeee cc -> clutch, eee-entry 
        // in this solution entry bit 1 is in bit 3, entry bit 0 is in bit 0
        unsigned in_grp = i >> 2;
        unsigned in_grpH = (in_grp & 0b1100) >> 2;
        unsigned in_grpL = in_grp & 0b0011;
        unsigned H = 0 == ((i & 0b0010) >> 1);
        unsigned L = i & 0b0001;
        // zle unsigned indx = (in_grpH << 4) | (L << 3) | (in_grpL << 1) | H;
        unsigned indx = (in_grpH << 4) | (H << 3) | (in_grpL << 1) | L;
        const auto& c = pal[i];
        unsigned char entry = (c.g << 5) | (c.r << 2) | c.b;
        _ASSERT(indx < 48);
        zx_pal[indx] = entry;
    }
    of << "const char " << prefix << "_palette[] = {" << std::hex << std::endl;

    for (unsigned i = 0; i < 3; i++)
    {
        for (unsigned j = 0; j < 16; j++)
        {
            of << "0x" << (int)zx_pal[i * 16 + j];
            if (j != 15 || i != 2)
                of << ", ";
        }
        of << std::endl;
    }
    of << "};" << std::endl;
}

*/


