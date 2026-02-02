#pragma once
#include "base.h"
#include <map>
#include <set>


class Nearest
{
public:
    Nearest(const uchar* palette, const cv::Vec3b* palette_tc, std::set<unsigned> excluded_indexes)
        : _palette{ palette },
        _palette_tc{ palette_tc ? palette_tc : _expanded_pal },
        _excluded_indexes{ excluded_indexes }
    {
        EnsureTC();
    }

    // returns entry, distance
    DistanceInfo GetNearest(const cv::Vec3b& point) const;

private:
    const uchar* _palette;
    const cv::Vec3b* _palette_tc;
    std::set<unsigned> _excluded_indexes;
    cv::Vec3b _expanded_pal[16];
    void EnsureTC();
};

