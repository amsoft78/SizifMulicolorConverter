#include "Saver.h"
#include <fstream>
#include <map>
#include <iostream>

Saver::Saver(const uchar* avail_zx_palette_as_rgb)
{
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

/* VERSION WITH COMPRESSSION
void Saver::Save(std::ofstream& of, unsigned page) const
{
    uchar* data = (page == 1) ? out_page1 : out_page0;
    unsigned i = 0;
    unsigned maxi = 192 * 32;
    unsigned lin_items = 0;
    std::map<uchar, unsigned> prefixes;
    
    uchar prefix_mask = 0xE0;
    uchar max_rep = ~prefix_mask + 1;

    for (i = 0; i < maxi; i++)
    {
        uchar curr = data[i];
        uchar pref = prefix_mask & curr;
        prefixes[pref]++;
    }
    std::map<unsigned, uchar> rev_prefixes;
    for (const auto& el : prefixes)
    {
        rev_prefixes[el.second] = el.first;
    }
    std::cout << rev_prefixes.begin()->first << " : " << std::hex << (int) rev_prefixes.begin()->second << std::dec << std::endl;
    
    auto choosen_prefix = rev_prefixes.begin()->second;
    of << "0x" << (unsigned)prefix_mask;
    of << ", 0x" << (unsigned)choosen_prefix;
    i = 0;
    int output_count = 0;
    while (i < maxi)
    {
        // check data receptibility
        uchar curr = data[i];
        unsigned rep = 1;
        while (i + rep < maxi && rep < max_rep && data[i + rep] == curr)
            rep++;
        // 0b11xxxxxx must be stored that way anyway
        if (rep > 2 || ((curr & prefix_mask) == choosen_prefix))
        {
            of << ", 0x" << (choosen_prefix | (rep - 1));
            of << ", 0x" << (unsigned)curr;
            lin_items++;
            i += rep;
            output_count += 2;
        }
        else
        {
            of << ", 0x" << (unsigned)curr;
            i++;
            output_count++;
        }
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
    std::cout << "Compressed data size: " << output_count << " for " << 256 * 192 / 8 << std::endl;
}
*/

void Saver::Save(std::ofstream& of, unsigned page) const
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


 