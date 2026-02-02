#include <fstream>
#include <iostream>
#include "SaverDual.h"
#include "Nearest.h"
#include "base.h"
#include "PaletteStatistics.h"

SaverDual::SaverDual(const uchar* avail_zx_palette_as_rgb)
    : Saver4(avail_zx_palette_as_rgb)
{
}

void SaverDual::SavePaletteAsAtributes(std::ofstream& of, const std::vector<RGB>& attribs, const std::string& prefix) const
{
    for (unsigned page = 0; page <= 1; page++)
    {
        of << "const char " << prefix << "_attribs" << page << "[] = {" << std::hex << std::endl;
        // there are two numbers for each entry!, unpack it.
        for (unsigned row = 0; row < 24; row++)
        {
            for (unsigned col = 0; col < 16; col += 1)
            {
                // save constant value for first plane
                unsigned char plane1_color = page == 0 ?
                    0x1C :
                    0xE0;
                of << "0x" << (int)(plane1_color) << ", ";
                auto indx = page == 1 ?
                    (row * 32 + col) * 2 :
                    (row * 32 + col) * 2 + 1;

                const auto& rgb = attribs[indx];
                auto packed = Pack(rgb);
                of << "0x" << (int)(packed);
                if (row != 23 || col != 31)
                    of << ", ";
            }
            of << std::endl;
        }
        of << "};" << std::endl;
    }
}

void SaverDual::SaveHeader(std::ofstream& of, const std::string& project)
{
    std::string fullname = project + "dp";
    Saver4::_SaveHeader(of, fullname);
}

void SaverDual::SaveCFile(std::ofstream& of, const std::string& project0, const std::vector<RGB>& attribs)
{
    std::string project = project0 + "dp";
    of << "#pragma bank ?" << std::endl;

    of << "#include \"" << project << ".h\"" << std::endl;
    of << "#include <string.h>" << std::endl;
    of << "const char " << project << "_page_1[] = {" << std::hex << std::endl;
    SaveScreenPage(of, 1);
    of << "};" << std::endl;
    SavePaletteAsAtributes(of, attribs, project);

    of << "void " << project << "_show()" << std::endl;
    of << "{" << std::endl;
    of << "    memcpy((void*)0x5800, " << project << "_attribs0, 768);" << std::endl;
    of << "    memcpy((void*)0x7800, " << project << "_attribs1, 768);" << std::endl;
    of << "    memset((void*)0x4000, 0xff, 6144);" << std::endl;
    of << "    memcpy((void*)0x6000, " << project << "_page_1, 6144);" << std::endl;
    of << "}" << std::endl;
    _Save4thColor(of, project);
}

void SaverDual::PutPixel(unsigned row, unsigned col, unsigned val)
{
    uint8_t triple_mask = row & 0xC0;
    uint8_t line_in_char = row & 0x07;
    uint8_t line_in_block = (row & 0x38) >> 3;
    uint16_t addr_by_row = (triple_mask + (line_in_char << 3) + line_in_block) << 5;
    uint8_t col_shift = (col >> 2) & 0x1F;
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
    // only page 1 is filled with "background" data
    out_page1[addr] = (out_page1[addr] & mask) | bits2; 
}

cv::Vec3b SaverDual::CodePixel(unsigned row, unsigned col, const cv::Vec3b& p, const std::vector<RGB>& pal_rgb, unsigned pal_indx_base)
{
    int dist_c0 = _g.col_global0_rgb.has_value()
        ? Dist(*_g.col_global0_rgb, p)
        : std::numeric_limits<int>::max();
    
    // 4th color
    const auto& color4 = _color4[row >> 3];
    int dist_c1 = Dist(color4, p);

    DistanceInfo nearest_palette;
    unsigned start_palette = pal_indx_base;

    if ((col % 8) < 4)
    {
        start_palette -= 2;
    }
    if (col >= 4)
        nearest_palette = NearestPal(pal_rgb, start_palette, 2, p); // the palette is updated when displaying pixel 4..7, 12..15 etc
    else
        nearest_palette = DistanceInfo{ 0 , std::numeric_limits<int>::max() };

    cv::Vec3b best;
    unsigned code = 0;
    if (dist_c0 <= dist_c1 && dist_c0 < nearest_palette.dinstance) // prefer local attributes, as they have 16 not 8 colors
    {
        best = Expand(*_g.col_global0);
        code = 0b01; // code for BACkGROUND color
        //second_plane = 0b00; // ignored zero
    }
    else if (dist_c1 < nearest_palette.dinstance)
    {
        best = Expand(color4); // *_g.col_global1);
        code = 0b11; // code for TIMEX color
    }
    else
    {
        best = Expand(pal_rgb[nearest_palette.indx + start_palette]);
        code = nearest_palette.indx << 1;
    }

    PutPixel(row, col, code);
    return best;
}
