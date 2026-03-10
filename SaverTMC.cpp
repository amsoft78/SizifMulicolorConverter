#include <fstream>
#include <iostream>
#include "SaverTMC.h"
#include "Nearest.h"
#include "base.h"
#include "PaletteStatistics.h"

SaverTMC::SaverTMC(const uchar* avail_zx_palette_as_rgb)
    : Saver(avail_zx_palette_as_rgb)
{
}

GlobalStat SaverTMC::AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in)
{
    // nothing to do in Timex Multicolor mode
    return _g;
}

void SaverTMC::AnalyzeWholeBuiltPalette(const std::vector<RGB>& pal_rgb)
{
    _ula_plus_palette.resize(64);
    // what we need is to have most popular pairs in one chunk
    std::map<uint16_t, unsigned> coded_pairs;
    for (unsigned i = 0; i < pal_rgb.size(); i += 2)
    {
        // code the map key on 16 bits
        uchar first = Pack(pal_rgb[i]);
        uchar second = Pack(pal_rgb[i + 1]);
        uint16_t key = (first << 8) | second;
        coded_pairs[key]++;
    }
    
    std::multimap<unsigned, int16_t> _rev_pairs;
    for (const auto& e : coded_pairs)
    {
        _rev_pairs.insert(std::make_pair(e.second, e.first));
    }

    // most offten pair goes first
    auto it_rev = _rev_pairs.rbegin();
    unsigned processed{ 0 };
    
    for (unsigned chunk = 0; chunk < 4; chunk++)
    {
        // now 8 inks and 8 papers in the chunk
        std::set<uchar> inks;
        std::set<uchar> papers;
        bool no_room{ false };
        while ( ! no_room
            && it_rev != _rev_pairs.rend())
        {
            _pair_code2chunk[it_rev->second] = chunk;
            uchar first = it_rev->second >> 8;
            uchar second = it_rev->second & 0xFF;
            if (inks.find(first) == inks.end() || papers.find(second) == papers.end())
            {
                if (inks.find(first) == inks.end())
                {
                    if (inks.size() < 8)
                        inks.insert(first);
                    else
                    {
                        //no_room = true;
                    }
                }
                if (papers.find(second) == papers.end())
                {
                    if (papers.size() < 8)
                        papers.insert(second);
                    else
                    {
                        //no_room = true;
                    }
                }
            }
            no_room = papers.size() == 8 and inks.size() == 8;
            if (!no_room)
            {
                it_rev++;
                processed++;
            }
        }
        // fill ULA+ entries
        auto it_i = inks.begin();
        for (unsigned ii = 0; ii < 8, it_i != inks.end(); ii++, ++it_i)
        {
            _ula_plus_palette[chunk * 16 + ii] = *it_i;
        }
        auto it_p = papers.begin();
        for (unsigned ip = 0; ip < 8, it_p != papers.end(); ip++, ++it_p)
        {
            _ula_plus_palette[chunk * 16 + 8 + ip] = *it_p;
        }
    }
    std::cout << "Processed color pairs: " << processed << std::endl;
}


uint16_t SaverTMC::GetByteAddressInPage(unsigned row, unsigned col) const
{
    uint8_t triple_mask = row & 0xC0;
    uint8_t line_in_char = row & 0x07;
    uint8_t line_in_block = (row & 0x38) >> 3;
    uint16_t addr_by_row = (triple_mask + (line_in_char << 3) + line_in_block) << 5;
    uint8_t col_shift = (col >> 3) & 0x1F;
    return addr_by_row + col_shift;
}

void SaverTMC::PutPixel(unsigned row, unsigned col, unsigned val)
{
    uint16_t addr = GetByteAddressInPage(row, col);
    // 0 1 2 ... 7  in byte
    unsigned char bits = 0x80 >> (col & 0x07);
    unsigned char mask = ~bits;
    unsigned char bits2 = val ? bits : 0;
    // all pixels goes to the first bank
    out_page0[addr] = (out_page0[addr] & mask) | bits2;
}

std::string SaverTMC::GetFullProjectName(const std::string& core) const
{
    return core+"tmc";
}

void SaverTMC::SavePaletteAsAtributes(std::ofstream& of, const std::vector<RGB>&, const std::string& prefix) const
{
}


cv::Vec3b SaverTMC::CodePixel(unsigned row, unsigned col,
    const cv::Vec3b& p,
    const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base)
{ 
    // build ULA+ palette if needed
    if (_up && row == 0 && col == 0)
    {
        AnalyzeWholeBuiltPalette(pal_rgb);
    }
    auto nearest_palette = NearestPal(pal_rgb, pal_indx_base, 2, p);
    static std::set<unsigned> pal_fixed;

    cv::Vec3b best;
    unsigned code = 0;
    if (_up)
    {
        uint16_t attr_addr = GetByteAddressInPage(row, col);
        unsigned chunk = (out_page1[attr_addr] & 0xC0) >> 6;
        unsigned up_ink_indx = out_page1[attr_addr] & 0x7;
        unsigned up_paper_indx = (out_page1[attr_addr] & 0x38) >> 3;
        if (pal_fixed.find(pal_indx_base) == pal_fixed.end())
        {
            // chunk is unknown
            uchar first = Pack(pal_rgb[pal_indx_base]);
            uchar second = Pack(pal_rgb[pal_indx_base + 1]);
            uint16_t key = (first << 8) | second;
            auto chunk_it = _pair_code2chunk.find(key);
            if (chunk_it != _pair_code2chunk.end())
            {
                chunk = chunk_it->second;
            }
            else
            {
                // no chunk contains this whole pair
                auto point_near_i = NearestPal(_ula_plus_palette, 0, 64,
                    p).indx;
                chunk = point_near_i >> 6;
            }
            // locate the ink
            up_ink_indx = NearestPal(_ula_plus_palette, chunk * 16, 8, 
                Expand(pal_rgb[pal_indx_base])).indx;
            up_paper_indx = NearestPal(_ula_plus_palette, chunk * 16 + 8, 8,
                Expand(pal_rgb[pal_indx_base+1])).indx;
            // update attributes in page 
            out_page1[attr_addr] = (chunk << 6)
                | (up_paper_indx << 3)
                | up_ink_indx;
            pal_fixed.insert(pal_indx_base);
        }

        unsigned chunk2 = (out_page1[attr_addr] & 0xC0) >> 6;
        unsigned up_ink_indx2 = out_page1[attr_addr] & 0x7;
        unsigned up_paper_indx2 = (out_page1[attr_addr] & 0x38) >> 3;
        assert(chunk == chunk2);
        assert(up_ink_indx2 == up_ink_indx);
        assert(up_paper_indx2 == up_paper_indx);
        // paper or ink?
        code = nearest_palette.indx == 0;
        
        if (code)
            best = Expand(_ula_plus_palette[chunk * 16 + up_ink_indx]);
        else
            best = Expand(_ula_plus_palette[chunk * 16 + 8 + up_paper_indx]);
    }
    else
    {
        // just find the proper ZX palette
        if (nearest_palette.indx == 0)
        {
            Nearest n_all{ _zx_palette, nullptr, std::set<unsigned>{} };
            auto tc_paper = Expand(pal_rgb[pal_indx_base]);
            auto paper = n_all.GetNearest(tc_paper);
            unsigned char paper_zx = paper.indx;
            best = Expand(_zx_palette[paper_zx]);
        }
        else
        {
            Nearest n_darks{ _zx_palette, nullptr,
                std::set<unsigned>{ 8, 9, 10, 11, 12, 13, 14, 15} };
            auto tc_ink = Expand(pal_rgb[pal_indx_base + 1]);
            auto ink = n_darks.GetNearest(tc_ink);
            unsigned char ink_zx = ink.indx;
            best = Expand(_zx_palette[ink_zx]);
        }
        code = nearest_palette.indx;
    }

    PutPixel(row, col, code);
    return best;

}

std::set<RGB> SaverTMC::UsePrevPaletteEntries(const std::vector<RGB>& /*pal_rgb*/,
    unsigned /*pal_indx_base*/,
    unsigned /*current_column*/,
    unsigned ) const
{
    std::set<RGB> arleady_avail;

    return arleady_avail;
}

void SaverTMC::SaveHeader(std::ofstream& of, const std::string& project0)
{
    std::string project = GetFullProjectName(project0);
    if (_g.col_global0_indx.has_value())
        of << "#define " << project << "col0 0x" << std::hex << *_g.col_global0_indx << std::endl;
    if (_g.col_global1_indx.has_value())
        of << "#define " << project << "col1 0x" << std::hex << *_g.col_global1_indx << std::endl;
    if (_g.col_global0_rgb.has_value())
        of << "#define " << project << "rgb0 0x" << std::hex << (int)Pack(*_g.col_global0_rgb) << std::endl;

    of << "extern void " << project << "_show() __banked;" << std::endl;
    of << "extern void " << project << "_prepare_colors() __banked;" << std::endl;
}

void SaverTMC::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    // first save attributes to page1
    // not much possibilitites without ULA+
    if (!_up)
    {
        Nearest n_all{ _zx_palette, nullptr, std::set<unsigned>{} };
        Nearest n_darks{ _zx_palette, nullptr,
            std::set<unsigned>{ 8, 9, 10, 11, 12, 13, 14, 15} };

        for (unsigned row = 0; row < 192; row++)
        {
            for (unsigned col = 0; col < 32; col++)
            {
                uint16_t addr = GetByteAddressInPage(row, col * 8);
                unsigned i = row * 32 + col;
                auto tc_paper = Expand(attribs[i * 2]);
                auto tc_ink = Expand(attribs[i * 2 + 1]);
                auto ink = n_darks.GetNearest(tc_ink);
                auto paper = n_all.GetNearest(tc_paper);
                unsigned char ink_zx = ink.indx;
                unsigned char paper_zx = paper.indx;
                bool bright_paper = paper_zx & 0x08;
                unsigned char code =
                    (bright_paper ? 0x40 : 0x00)
                    | (paper_zx << 3)
                    | (ink_zx)
                    ;
                out_page1[addr] = code;
            }
        }
    }
    std::string fullname = GetFullProjectName(project);
    Saver::SaveCFile(of, fullname, attribs);
    if (_up)
    {
        of << "void SetColor(unsigned char entry, unsigned char value);" << std::endl;
        of << "void " << fullname << "_prepare_colors()" << std::endl;
        of << "{" << std::endl;
        for (unsigned uplus_index = 0; uplus_index < 64; uplus_index++)
            of << "    SetColor(" << std::dec << uplus_index << ", 0x"
                << std::hex << (int)(_ula_plus_palette[uplus_index]) << ");" << std::endl;
        of << "}" << std::endl;
    }
}

