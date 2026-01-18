#include "Saver16.h"
#include "Nearest.h"
#include <fstream>
#include <iostream>

Saver16::Saver16(const uchar* avail_zx_palette_as_rgb)
    : Saver(avail_zx_palette_as_rgb)
{
   _nearest12 = std::make_unique<Nearest> (_zx_palette, nullptr, 
       std::set<unsigned>{0b0001 , 0b0101, 0b1000, 0b1100 } );
}

Saver16::~Saver16()
{
}

GlobalStat Saver16::AnalyzeGlobal(const cv::Vec3b*, const cv::Mat&)
{
    // nothing to do
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
    // start of the avalaible entries. we can use two previous colors because of event/odd overlaping
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
        //if ((col & 0x04) && entries_to_check > 2)
        //    code = code ^ 0x2;
        switch (code)
        {
        case 0b00:
            zx_col = 0b0001;
            break;
        case 0b01:
            zx_col = 0b0101;
            break;
        case 0b10:
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

    return out_pixel_color;
}

std::set<RGB> Saver16::UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb, unsigned pal_indx_base, unsigned current_column) const
{
    std::set<RGB> arleady_avail;
    if (current_column < 4)
        return arleady_avail;
    arleady_avail.insert(pal_rgb[pal_indx_base - 1]);
    arleady_avail.insert(pal_rgb[pal_indx_base - 2]);
    return arleady_avail;
}

void Saver16::SavePalette(std::ofstream& of, const std::vector<RGB> pal, const std::string& prefix, const RGB* others) const
    {
        unsigned zx_pal[64];
        if (others)
        {
            for (unsigned i = 0; i < 8; i++)
            zx_pal[i+8] = Pack(others[i]);
        }
        for (unsigned i = 0; i < 48; i++)
        {
            // addressing bits are swapped, bit 3 swaps ink / paper
            // 00ccPeee cc -> clutch, eee-entry 
            // in this solution entry bit 1 is in bit 3, entry bit 0 is in bit 0
            unsigned in_grp = i >> 2;

            // groups of 8 lines are negative, so the sections are in order 11, 10, 01. Section 0 holds only border color!
            unsigned in_grpH = ~(in_grp >> 2 ) & 0b0011; // (in_grp & 0b1100) >> 2;
            unsigned in_grpL = in_grp & 0b0011;
            unsigned H = 0 == ((i & 0b0010) >> 1);
            unsigned L = i & 0b0001;
           
            unsigned indx = (in_grpH << 4) | (H << 3) | (in_grpL << 1) | L;
            const auto& c = pal[i];
            unsigned char entry = Pack(c);
            _ASSERT(indx > 7 && indx <= 63);
            zx_pal[indx] = entry;
        }
        of << "const char " << prefix << "_palette[] = {" << std::hex << std::endl;

        for (unsigned i = 0; i < 4; i++)
        {
            for (unsigned j = 0; j < 16; j++)
            {
                if (j != 0 || i != 0)
                    of << ", ";
                of << "0x" << (int)((unsigned char)zx_pal[i * 16 + j]);

            }
            of << std::endl;
        }
        of << "};" << std::endl;
    }


