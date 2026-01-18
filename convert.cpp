#include "Nearest.h"

#include <iostream>
#include <fstream>

#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"

#include "PaletteStatistics.h"
#include "Saver4.h"
#include "Saver16.h"

#include <optional>
#include <bitset>
#include "C64.h"


void Convert(const std::string& input_file, const std::string& project, const int output_mode)
{
    //ColorInfo col_info[16];
    const auto avail_palette_16 = spectrum_more_rg;
    cv::Vec3b TC[16];


        for (unsigned j = 0; j < 16; j++)
        {
            //col_info[j].tc = Expand(avail_palette_16[j]);
            TC[j] = Expand(avail_palette_16[j]);
            //col_info[j].rgb256 = avail_palette_16[j];
        }

    const cv::Vec3b* avail_paletteTC = TC;

    //ShowPalette(col_info);
    
    cv::Mat in;


    const int ZX_SIZE_X = output_mode == 16  ? 128 : 256;
    bool is_commodore{ false };
    // determine file type

    if (input_file.find(".fli") != std::string::npos ||
        input_file.find(".bml") != std::string::npos ||
        input_file.find(".prg") != std::string::npos)
    {
        // Commodore FLI loader 
        in = C64::ReadCommodoreFLI(input_file);
        is_commodore = true;
    }
    else
    {
        // openCV auto file load
        in = cv::imread(input_file, cv::IMREAD_COLOR);
    }

    bool need_global = output_mode == 4;

    std::unique_ptr<Saver> saver;

    if (output_mode == 16)
        saver = std::make_unique <Saver16>(avail_palette_16);
    else
        if (output_mode == 4)
            saver = std::make_unique <Saver4>(avail_palette_16);
        else
            return;

    GlobalStat gs = saver->AnalyzeGlobal(avail_paletteTC, in);


    cv::Mat outsc(ZX_SIZE_X, 192, CV_8UC3, cv::Scalar(0, 0, 0)); // TODO - use most popular color!
    double factor = 1.0;

    double factor_y = 1.0;

    if (in.cols < ZX_SIZE_X * 0.625)
    {
        while (in.cols * factor < ZX_SIZE_X * 0.625)
        {
            factor = factor * 2;
            if (in.rows * factor_y <= 128)
                factor_y = factor_y * 2;
        }
    }
    else if (in.cols * factor > ZX_SIZE_X * 1.25)
    {
        while (in.cols * factor > ZX_SIZE_X * 1.7)
        {
            factor = factor / 2;
            if (in.rows * factor_y > 256)
                factor_y = factor_y / 2;
        }
    }
    cv::resize(in, in, cv::Size{}, factor, factor_y);

    // cut from the middle from wider images
    // cut from the middle from wider images
    int x_diff = in.cols - ZX_SIZE_X;
    int x_start = 0;
    if (x_diff > 0)
        x_start = x_diff / 2;
    int y_diff = in.rows - 192;
    int y_start = 0;
    if (y_diff > 0)
        y_start = y_diff / 2;
    in(cv::Rect(x_start, y_start, std::min(in.cols, (int)ZX_SIZE_X), std::min(in.rows, 192))).copyTo(outsc);


    // now statistics for 8x8
    unsigned rg = 0;
    unsigned cg = 0;

    std::vector<RGB> vec_pal;
    std::vector<unsigned> vec_attrib;
    vec_pal.resize(768*2);
    vec_attrib.resize(768 * 2);
 

    cv::Mat outzx(192, ZX_SIZE_X, CV_8UC3, cv::Scalar(0, 0, 0));

    for (unsigned r = 0; r < outsc.rows && r < 192; r += saver->RowsInGroup())
    {
        cg = 0;
        for (unsigned c = 0; c < outsc.cols && c < ZX_SIZE_X; c += saver->ColsInGroup())
        {
            std::cout << "Analyzing row group " << rg << ", column group " << cg << std::endl;

            auto pal_indx_base = ((rg << 5) + cg) << 1;

            auto nearest = Nearest{ avail_palette_16, avail_paletteTC, {} };
            unsigned cols_to_analyze = saver->ColsInAnalyzedGroup();
            if (c + cols_to_analyze > outsc.cols)
                cols_to_analyze = cols_to_analyze - c;
            PaletteStatistics ps{ saver->RowsInGroup(),
                            cols_to_analyze,
                            outsc,
                            nearest, r, c };

            auto& rev_col_map = ps.GetStat();

            std::set<RGB> arleady_avail = saver->UsePrevPaletteEntries(vec_pal, pal_indx_base, c);

            auto it_s = rev_col_map.rbegin();
            unsigned k = 0;
            for (;
                (it_s != rev_col_map.rend());
                ++it_s)
            {
                auto is_popular = arleady_avail.find(it_s->second.rgb);
                if (is_popular != arleady_avail.end())
                    continue;
                std::cout << "    count: " << it_s->first
                    << ", r: " << (int)it_s->second.rgb.r
                    << ", g: " << (int)it_s->second.rgb.g
                    << ", b: " << (int)it_s->second.rgb.b
                    << std::hex
                    << ", COL: 0x" << ((it_s->second.rgb.g << 5) + (it_s->second.rgb.r << 2) + it_s->second.rgb.b)
                    << std::dec
                    << ", ZX color " << it_s->second.entry_indx << ", dist " << it_s->second.entry_distance;
                    
                if (k < 2)
                {
                    auto indx = pal_indx_base + k;
                    _ASSERT(indx < 768 * 2);
                    //vec_pal[indx] = ToRGB(Expand(avail_palette_16[it_s->second.entry_indx])); // NOT it_s->second.rgb;
                    vec_pal[indx] = it_s->second.rgb;
                    vec_attrib[indx] = it_s->second.entry_indx;
                    k++;
                    std::cout << "*";
                    arleady_avail.insert(it_s->second.rgb);
                }
                std::cout << std::endl;
            }

            // fill the unused entries with already known value
            while (k < 2 && arleady_avail.size())
            {
                auto indx = pal_indx_base + k;
                _ASSERT(indx < 768 * 2);
                vec_pal[indx] = *arleady_avail.begin();
                k++;
            }

            for (int r1 = 0; r1 < saver->RowsInGroup() && (r + r1) < 192 && (r + r1) < outsc.rows; r1++)
            {
                for (int c1 = 0; c1 < saver->ColsInGroup() && (c + c1) < ZX_SIZE_X && (c + c1) < outsc.cols; c1++)
                {
                    const auto& p = outsc.at<cv::Vec3b>(r + r1, c + c1);
                    
                    
                    auto best = saver->CodePixel(r + r1, c + c1, p, vec_pal, pal_indx_base);

                    outzx.at<cv::Vec3b>(r+r1, c+c1) = best;
                }
            }

            cg++;
        }
        rg++;
    }
 
    std::ofstream ofm{ std::string(input_file).append(".h") };
    //ofm << "#define " << argv[2] << "col0 0x" << *col_global0_indx << std::endl;
    //ofm << "#define " << argv[2] << "rgb0 0x" << (int)Pack(rgb_most_popular) << std::endl;
    //ofm << "#define " << argv[2] << "col3 0x" << *col_global1_indx << std::endl;

    //ofm << "extern void " << argv[2] << "_show() __banked;" << std::endl;
    //ofm.close();

    //ofm.open(std::string(input_file).append(".c"), std::ios::trunc);
    // ofm << "#pragma bank ?" << std::endl;
    /*
    ofm << "#include \"" << project << ".h\"" << std::endl;
    ofm << "#include <string.h>" << std::endl;
    ofm << "const char " << argv[2] << "_page_0[] = {" << std::hex << std::endl;
    saver->Save(ofm, 0);
    ofm << "};" << std::endl;
    ofm << "const char " << argv[2] << "_page_1[] = {" << std::hex << std::endl;
    saver->Save(ofm, 1);
    ofm << "};" << std::endl;
    saver->SavePaletteAsAtributes(ofm, vec_pal, argv[2]);

    ofm << "void " << argv[2] << "_show()" << std::endl;
    ofm << "{" << std::endl;
    ofm << "    memcpy((void*)0x5800, " << argv[2] << "_attribs0, 768);" << std::endl;
    ofm << "    memcpy((void*)0x7800, " << argv[2] << "_attribs1, 768);" << std::endl;
    ofm << "    memcpy((void*)0x4000, " << argv[2] << "_page_0, 6144);" << std::endl;
    ofm << "    memcpy((void*)0x6000, " << argv[2] << "_page_1, 6144);" << std::endl;
    ofm << "}" << std::endl;
    ofm.close();
    */
    cv::Mat imzx;
    cv::resize(outzx, imzx, cv::Size(), ZX_SIZE_X < 256 ? 8 : 4, 4);
    auto fnamezx = std::string(input_file).append("_zx.bmp");
    cv::imwrite(fnamezx, outzx);

    cv::Mat im;
    cv::resize(outsc, im, cv::Size(), ZX_SIZE_X < 256 ? 8 : 4, 4);
    auto fname = std::string(input_file).append(".bmp");
    cv::imwrite(fname, in);
    cv::imshow(input_file, im);
    cv::imshow("zx", imzx);

    cv::waitKey(0);
}


int main(int argc, char* argv[])
{
    if (argc < 3)
        return -1;

    // 4 or 16 colors
    const int output_mode = 16;
    Convert(argv[1], argv[2], output_mode);
}