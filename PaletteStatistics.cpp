#include "PaletteStatistics.h"
#include "Nearest.h"
#include <iostream>

PaletteStatistics::PaletteStatistics(
    unsigned group_pixels_rows,
    unsigned group_pixels_cols,
    const cv::Mat& image,
    const Nearest& dist_measure,
    unsigned r0,
    unsigned c0)

{
    std::map<RGB, unsigned> col_map;
    for (int c1 = 0; c1 < group_pixels_cols && (c0 + c1) < 256 && (c0 + c1) < image.cols; c1++)
    {
        for (int r1 = 0; r1 < group_pixels_rows && (r0 + r1) < 192 && (r0 + r1) < image.rows; r1++)
        {
            const auto& p = image.at<cv::Vec3b>(r0 + r1, c0 + c1);

            // check only older 3 bits
            RGB col = ToRGB(p);

            col_map[col]++;
        }
    }

    for (const auto& it : col_map)
    {
        cv::Vec3b color_point{
            ExpandB(it.first.b),
            ExpandRG(it.first.g),
            ExpandRG(it.first.r)
        };
        auto near = dist_measure.GetNearest(color_point);
        auto stat = ColorStatInfo{ it.first, near.indx, near.dinstance };
        _rev_col_map.insert(std::make_pair(it.second, stat));
    }
    /*
    auto it_s = _rev_col_map.rbegin();
    for (unsigned k = 0, k16 = 0;
        (k < 4 || k16 < 4) && it_s != _rev_col_map.rend();
        ++k, ++it_s)
    {
        cv::Vec3b color_point{
            ExpandB(it_s->second.b),
            ExpandRG(it_s->second.g),
            ExpandRG(it_s->second.r)
        };

        auto near = dist_measure.GetNearest(color_point);

        std::cout << "    count: " << it_s->first
            << ", r: " << (int)it_s->second.r
            << ", g: " << (int)it_s->second.g
            << ", b: " << (int)it_s->second.b
            << std::hex
            << ", COL: 0x" << ((it_s->second.g << 5) + (it_s->second.r << 3) + it_s->second.b)
            << std::dec
            << ", ZX color " << near.first << ", dist " << near.second
            << std::endl;
    }
    */
}

void PaletteStatistics::RunZxStatistics()
{
}

void PaletteStatistics::RunRGBStatistics()
{
}
