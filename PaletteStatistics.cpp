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
}

void PaletteStatistics::RunZxStatistics()
{
}

void PaletteStatistics::RunRGBStatistics()
{
}
