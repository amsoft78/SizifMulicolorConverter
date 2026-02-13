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
#include "SaverDual.h"
#include "SaverHR.h"

#include <optional>
#include <bitset>
#include "C64.h"

enum class OutputMode
{
    cga4,
    cga16,
    dual_playfield,
    timex_hr,
};

void Convert(const std::string& input_file, const std::string& project, const OutputMode output_mode)
{
    const auto avail_palette_16 = spectrum_more_rg;
    cv::Vec3b TC[16];


        for (unsigned j = 0; j < 16; j++)
        {
            TC[j] = Expand(avail_palette_16[j]);
        }

    const cv::Vec3b* avail_paletteTC = TC;
    
    cv::Mat in;

    std::unique_ptr<Saver> saver;
    std::string output_mode_prefix;

    switch (output_mode)
    {
    case OutputMode::cga16:
        saver = std::make_unique <Saver16>(avail_palette_16);
        output_mode_prefix = "16";
        break;
    case OutputMode::cga4:
        saver = std::make_unique <Saver4>(avail_palette_16);
        output_mode_prefix = "4";
        break;
    case OutputMode::dual_playfield:
        saver = std::make_unique <SaverDual>(avail_palette_16);
        output_mode_prefix = "dp";
        break;
    case OutputMode::timex_hr:
        saver = std::make_unique <SaverHR>();
        output_mode_prefix = "hr";
        break;
    default:
        return;
    }

    const int ZX_SIZE_X = saver->ScreenColumns();
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


    cv::Mat outsc(ZX_SIZE_X, 192, CV_8UC3, cv::Scalar(0, 0, 0)); // TODO - use most popular color!
    double factor = 1.0;

    double factor_y = 1.0;

    if (in.cols <= ZX_SIZE_X * 0.625)
    {
        while (in.cols * factor <= ZX_SIZE_X * 0.625)
        {
            factor = factor * 2;
            if (in.rows * factor_y <= 128)
                factor_y = factor_y * 2;
        }
    }
    else if (in.cols * factor >= ZX_SIZE_X * 1.25)
    {
        while (in.cols * factor > ZX_SIZE_X * 1.25)
        {
            factor = factor / 2;
            if (in.rows * factor_y > 256)
                factor_y = factor_y / 2;
        }
    }
    cv::resize(in, in, cv::Size{}, factor, factor_y);

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

    GlobalStat gs = saver->AnalyzeGlobal(avail_paletteTC, outsc);

    // now statistics for 8x8
    unsigned rg = 0;
    unsigned cg = 0;

    std::vector<RGB> vec_pal;
    vec_pal.resize(768*2);


    cv::Mat outzx(192, ZX_SIZE_X, CV_8UC3, cv::Scalar(0, 0, 0));

    unsigned free_prev_palette_items{ 0 }; // if prevoius group did not generate both entries, it might be used by next group

    if (saver->RowsInGroup() > 1 && saver->ColsInGroup() > 1)
    for (unsigned r = 0; r < outsc.rows && r < 192; r += saver->RowsInGroup(), rg++)
    {
        cg = 0;
        for (unsigned c = 0; c < outsc.cols && c < ZX_SIZE_X; c += saver->ColsInGroup(), cg++)
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

            std::set<RGB> arleady_avail = saver->UsePrevPaletteEntries(vec_pal, pal_indx_base, c, r);

            auto it_s = rev_col_map.rbegin();
            int to_fill = 2 + free_prev_palette_items;
            int insert_cout{ 0 };
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
                if (to_fill > 0 && !(it_s->second.entry_distance == 0 && saver->CanUseNativeZXEntry(it_s->second.entry_indx) ))
                {
                    auto indx = pal_indx_base - free_prev_palette_items + to_fill - 1;
                    _ASSERT(indx < 768 * 2);
                    vec_pal[indx] = it_s->second.rgb;
                    to_fill--;
                    std::cout << "*";
                    arleady_avail.insert(it_s->second.rgb);
                    insert_cout++;
                    if (insert_cout > 2)
                        std::cout << "***";
                }

                std::cout << std::endl;
            }

            // allow use not filled entries by a next colour group
            if (to_fill > 1)
            {
                //free_prev_palette_items = std::min(to_fill, 2);
            }
        }
    }

    rg = 0;
    for (unsigned r = 0; r < outsc.rows && r < 192; r += saver->RowsInGroup(), rg++)
    {
        cg = 0;
        for (unsigned c = 0; c < outsc.cols && c < ZX_SIZE_X; c += saver->ColsInGroup(), cg++)
        {
            auto pal_indx_base = ((rg << 5) + cg) << 1;

            for (int r1 = 0; r1 < saver->RowsInGroup() && (r + r1) < 192 && (r + r1) < outsc.rows; r1++)
            {
                for (int c1 = 0; c1 < saver->ColsInGroup() && (c + c1) < ZX_SIZE_X && (c + c1) < outsc.cols; c1++)
                {
                    const auto& p = outsc.at<cv::Vec3b>(r + r1, c + c1);

                    auto best = saver->CodePixel(r + r1, c + c1, p, vec_pal, pal_indx_base);
                    outzx.at<cv::Vec3b>(r + r1, c + c1) = best;
                }
            }
        }
    }
 
    auto fn_base = input_file;
    auto pos_bsh = input_file.find_last_of('\\');
    if (pos_bsh != std::string::npos)
    {
        fn_base = input_file.substr(0, pos_bsh+1);
        fn_base += project;
    }

    std::stringstream ss;
    ss << fn_base << output_mode_prefix << ".h";

    std::ofstream ofm{ ss.str()};
    saver->SaveHeader(ofm, project);
    ofm.close();

    ss.str("");
    ss << fn_base << output_mode_prefix << ".c";

    ofm.open(ss.str(), std::ios::trunc);
    saver->SaveCFile(ofm, project, vec_pal);

    ofm.close();
    
    cv::Mat imzx;
    auto factorx = ZX_SIZE_X < 256 ? 8 : (ZX_SIZE_X < 512 ? 4 : 2);
    cv::resize(outzx, imzx, cv::Size(), factorx, 4);
    ss.str("");
    ss << fn_base << output_mode_prefix << "_zx.bmp";
    cv::imwrite(ss.str(), outzx);

    cv::Mat im;
    cv::resize(outsc, im, cv::Size(), factorx, 4);
    ss.str("");
    ss << fn_base << output_mode_prefix << "_preview.bmp";;
    cv::imwrite(ss.str(), in);
    cv::imshow(input_file, im);
    cv::imshow("zx", imzx);

    cv::waitKey(0);
}


int main(int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cerr << "Usage: " << argv[0] << " file_path project_name color mode (4 or 16)" << std::endl;
        return -1;
    }

    // 4 or 16 colors
    std::string str_om{ argv[3] };
    OutputMode output_mode;
    if (str_om == "4")
        output_mode = OutputMode::cga4;
    else if (str_om == "16")
        output_mode = OutputMode::cga16;
    else if (str_om == "dual")
        output_mode = OutputMode::dual_playfield;
    else if (str_om == "hr")
        output_mode = OutputMode::timex_hr;
    else
    {
        std::cerr << "Output mode ca be only 4 or 16 or dual" << std::endl;
        return -1;
    }

    Convert(argv[1], argv[2], output_mode);
}