#include "Saver.h"
#include <fstream>
#include <map>
#include <iostream>

Saver::Saver(const uchar* avail_zx_palette_as_rgb)
{
    if (avail_zx_palette_as_rgb)
        memcpy(_zx_palette, avail_zx_palette_as_rgb, 16);
    out_page0 = new uchar[32 * 192];
    out_page1 = new uchar[32 * 192];
    memset(out_page0, 0, 32 * 192);
    memset(out_page1, 0, 32 * 192);
}

Saver::~Saver()
{
    delete[] out_page0;
    delete[] out_page1;
}


void Saver::SaveScreenPage(std::ofstream& of, unsigned page) const
{
    uchar* data = (page == 1) ? out_page1 : out_page0;
    unsigned i = 0;
    unsigned maxi = 192 * 32;
    unsigned lin_items = 0;

    int output_count = 0;
    for (unsigned i = 0; i < maxi; i++)
    {
        // check data receptibility
        uchar curr = data[i];
        of << "0x" << (unsigned)curr;
        if (i < maxi - 1)
            of << ", ";
        output_count++;
        if (i < maxi - 1)
        {
            lin_items++;
            if (lin_items > 20)
            {
                of << std::endl;
                lin_items = 0;
            }
        }
        else
        {
            of << std::endl;
        }
    }
}

void Saver::SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs)
{
    of << "#pragma bank ?" << std::endl;

    of << "#include \"" << project << ".h\"" << std::endl;
    of << "#include <string.h>" << std::endl;
    of << "const char " << project << "_page_0[] = {" << std::hex << std::endl;
    SaveScreenPage(of, 0);
    of << "};" << std::endl;
    of << "const char " << project << "_page_1[] = {" << std::hex << std::endl;
    SaveScreenPage(of, 1);
    of << "};" << std::endl;
    SavePaletteAsAtributes(of, attribs, project);

    of << "void " << project << "_show()" << std::endl;
    of << "{" << std::endl;
    of << "    memcpy((void*)0x5800, " << project << "_attribs0, 768);" << std::endl;
    of << "    memcpy((void*)0x7800, " << project << "_attribs1, 768);" << std::endl;
    of << "    memcpy((void*)0x4000, " << project << "_page_0, 6144);" << std::endl;
    of << "    memcpy((void*)0x6000, " << project << "_page_1, 6144);" << std::endl;
    of << "}" << std::endl;
}

void Saver::SavePaletteAsAtributes(std::ofstream& of,
    const std::vector<RGB>& attribs,
    const std::string& prefix) const
{
    for (unsigned page = 0; page <= 1; page++)
    {
        of << "const char " << prefix << "_attribs" << page << "[] = {" << std::hex << std::endl;
        // there are two numbers for each entry!, unpack it.
        for (unsigned row = 0; row < 24; row++)
        {
            for (unsigned col = 0; col < 32; col += 1)
            {
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
