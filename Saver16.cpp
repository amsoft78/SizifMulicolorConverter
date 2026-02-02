#include "Saver16.h"
#include "Nearest.h"
#include "PaletteStatistics.h"
#include <fstream>
#include <iostream>

Saver16::Saver16(const uchar* avail_zx_palette_as_rgb)
    : Saver(avail_zx_palette_as_rgb)
{
    _nearest12 = std::make_unique<Nearest>(_zx_palette, nullptr,
        std::set<unsigned>{0b0001, 0b0101, 0b1000, 0b1100 } );
}

Saver16::~Saver16()
{
}

GlobalStat Saver16::AnalyzeGlobal(const cv::Vec3b*, const cv::Mat& in)
{
    auto nearest_low = Nearest{ _zx_palette, nullptr, {8, 9, 10, 11, 12, 13, 14, 15} };

    PaletteStatistics ps{ unsigned(in.rows), (unsigned)in.cols,
                    in,
                    nearest_low, 0, 0 };
    auto& rev_col_map_low = ps.GetStat();
    auto it_s = rev_col_map_low.rbegin();
    if (it_s != rev_col_map_low.rend())
    {
        _border_color = it_s->second.entry_indx;
        _border_rgb = it_s->second.rgb;
    }
    // nothing to return
    return GlobalStat();
}

void Saver16::PutPixel(unsigned row, unsigned col, unsigned val)
    {
        uint8_t triple_mask = row & 0xC0;
        uint8_t line_in_char = row & 0x07;
        uint8_t line_in_block = (row & 0x38) >> 3;
        uint16_t addr_by_row = (triple_mask + (line_in_char << 3) + line_in_block) << 5;
        uint8_t col_shift = (col >> 2) & 0x1F; 
        uint16_t addr = addr_by_row + col_shift;
        // even / odd
        unsigned char mask = (col & 0x01) ? 0xF0 : 0x0F;
        unsigned bits4 = (col & 0x01) ? val : (val << 4);
        // pixels 0 and 1 goes to first bank, 2 and 3 to the second
        if ((col & 0x02) == 0)
        {
            out_page0[addr] = (out_page0[addr] & mask) | bits4;
        }
        else
        {
            out_page1[addr] = (out_page1[addr] & mask) | bits4;
        }
    }

cv::Vec3b Saver16::CodePixel(unsigned row, unsigned col,
    const cv::Vec3b& p,
    const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base)
{
    auto nearest_native = _nearest12->GetNearest(p);
    // start of the available entries. we can use two previous colors because of event/odd overlapping
    unsigned start_palette = pal_indx_base;
    unsigned entries_to_check = 2;
    if (col >= 4 && pal_indx_base >= 2)
    {
        start_palette -= 2;
        entries_to_check = 4;
    }

    auto nearest_palette = NearestPal(pal_rgb, start_palette, entries_to_check, p);
    
    uchar zx_col = nearest_native.indx;

    cv::Vec3b out_pixel_color = Expand(_zx_palette[zx_col]);

    if (nearest_palette.dinstance < nearest_native.dinstance)
    {
        auto code = nearest_palette.indx;
        // re-code because of shifting!!!! odds/even column fours
        if ((col % 8) < 4 && col >= 8)
            code = code ^ 0x2;
        switch (code)
        {
        case 0b00:
            zx_col = 0b0001;
            break;
        case 0b10:
            zx_col = 0b0101;
            break;
        case 0b01:
            zx_col = 0b1000;
            break;
        case 0b11:
            zx_col = 0b1100;
            break;
        default:
            throw std::runtime_error("incorrect!");
        }
        out_pixel_color = Expand(pal_rgb[start_palette + nearest_palette.indx]);
    }
    PutPixel(row, col, zx_col);

    return out_pixel_color;
}

std::set<RGB> Saver16::UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base,
    unsigned current_column,
    unsigned current_row) const
{
    std::set<RGB> arleady_avail;

    std::set<unsigned> substituted_palette_entries = { 0b0001, 0b0101, 0b1000, 0b1100 };
    // can use 12 of 16 native colors
    for (unsigned i = 0; i < 16; i++)
    {
        if (substituted_palette_entries.find(i) == substituted_palette_entries.cend())
            arleady_avail.insert(ToRGB(Expand(_zx_palette[i])));
    }

    // can usetwo previously defined colors
    if (current_column < 4)
        return arleady_avail;
    arleady_avail.insert(pal_rgb[pal_indx_base - 1]);
    arleady_avail.insert(pal_rgb[pal_indx_base - 2]);
    return arleady_avail;
}

bool Saver16::CanUseNativeZXEntry(unsigned zx_color)
{
    // not-substituted palete entries can be used directly 
    std::set<unsigned> substituted_palette_entries = { 0b0001, 0b0101, 0b1000, 0b1100 };
    return substituted_palette_entries.find(zx_color) == substituted_palette_entries.cend();
}

void Saver16::SaveHeader(std::ofstream& of, const std::string& project)
{
    of << "#define " << project << "16col0 0x" << std::hex << _border_color << std::endl;
    of << "#define " << project << "16rgb0 0x" << std::hex << (int)Pack(_border_rgb) << std::endl;
 
    of << "extern void " << project << "16_show() __banked;" << std::endl;
}

void Saver16::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    std::string fullname = project + "16";
    Saver::SaveCFile(of, fullname, attribs);
}

