#pragma once
#include "base.h"
#include <map>

class Nearest;

struct ColorStatInfo
{
    RGB rgb;
    unsigned entry_indx;
    int entry_distance;
};

class PaletteStatistics
{
public:
    PaletteStatistics(
        unsigned group_pixels_rows,
        unsigned group_pixels_cols,
        const cv::Mat& image,
        const Nearest& dist_measure,
        unsigned r0,
        unsigned c0
    );
    const std::multimap<unsigned, ColorStatInfo>& GetStat() const
    {
        return _rev_col_map;
    }

private:
    std::multimap<unsigned, ColorStatInfo> _rev_col_map;

    void RunZxStatistics();
    void RunRGBStatistics();
};

