#include <fstream>
#include <iostream>
#include "SaverHR.h"
#include "Nearest.h"
#include "base.h"
#include "PaletteStatistics.h"

SaverHR::SaverHR()
    : Saver(nullptr)
{
}

GlobalStat SaverHR::AnalyzeGlobal(const cv::Vec3b*, const cv::Mat&)
{   
    return GlobalStat{};
}

void SaverHR::PutPixel(unsigned row, unsigned col, unsigned val)
{
    uint8_t triple_mask = row & 0xC0;
    uint8_t line_in_char = row & 0x07;
    uint8_t line_in_block = (row & 0x38) >> 3;
    uint16_t addr_by_row = (triple_mask + (line_in_char << 3) + line_in_block) << 5;
    uint8_t col_shift = col >> 4;
    uint16_t addr = addr_by_row + col_shift;
    auto bit_no = col & 0x07;
    unsigned char mask = 0x80 >> bit_no;
    unsigned bit =  val ? 0 : mask; // black puts ink (1)
    mask = ~mask;

    // pixels 0-7 goes to first bank, 8-15 to second
    if ((col & 0x08) == 0)
    {
        out_page0[addr] = (out_page0[addr] & mask) | bit;
    }
    else
    {
        out_page1[addr] = (out_page1[addr] & mask) | bit;
    }
}

cv::Vec3b SaverHR::CodePixel(unsigned row, unsigned col,
    const cv::Vec3b& p,
    const std::vector<RGB>& pal_rgb,
    unsigned pal_indx_base)
{
    uchar blue = p[0];
    uchar green = p[1];
    uchar red = p[2];

    auto Y = 0.299 * red + 0.587 * green + 0.114 * blue;

    unsigned int val = Y > 0.5 ? 1 : 0;

    auto best = val ? cv::Vec3b{ 255, 255, 255 } : cv::Vec3b{ 0,0,0 };
 
    PutPixel(row, col, val);
    return best;

}

std::set<RGB> SaverHR::UsePrevPaletteEntries(const std::vector<RGB>& /*pal_rgb*/,
    unsigned /*pal_indx_base*/,
    unsigned /*current_column*/,
    unsigned /*current_row*/) const
{
    std::set<RGB> arleady_avail;
    return arleady_avail;
}

/*
void Saver4::_SaveHeader(std::ofstream& of, const std::string& project) const
{
    of << "#define " << project << "col0 0x" << std::hex << *_g.col_global0_indx << std::endl;
    of << "#define " << project << "rgb0 0x" << std::hex << (int)Pack(*_g.col_global0_rgb) << std::endl;

    of << "extern void " << project << "_show() __banked;" << std::endl;
    of << "extern void " << project << "_prepare_colors() __banked;" << std::endl;
}
*/
void SaverHR::SaveHeader(std::ofstream& of, const std::string& project0)
{
    std::string project = project0 + "hr";
    of << "extern void " << project << "_show() __banked;" << std::endl;
}

void SaverHR::SaveCFile(std::ofstream& of, const std::string& project0, const std::vector<RGB>& attribs)
{
    std::string project = project0 + "hr";

    of << "#pragma bank ?" << std::endl;

    of << "#include \"" << project << ".h\"" << std::endl;
    of << "#include <string.h>" << std::endl;
    of << "const char " << project << "_page_0[] = {" << std::hex << std::endl;
    SaveScreenPage(of, 0);
    of << "};" << std::endl;
    of << "const char " << project << "_page_1[] = {" << std::hex << std::endl;
    SaveScreenPage(of, 1);
    of << "};" << std::endl;

    of << "void " << project << "_show()" << std::endl;
    of << "{" << std::endl;
    of << "    memcpy((void*)0x4000, " << project << "_page_0, 6144);" << std::endl;
    of << "    memcpy((void*)0x6000, " << project << "_page_1, 6144);" << std::endl;
    of << "}" << std::endl;
}
