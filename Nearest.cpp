#include "Nearest.h"

DistanceInfo Nearest::GetNearest(const cv::Vec3b& point) const
{
    unsigned res = 0;
    int min_dist = std::numeric_limits<int>::max();
    for (int i = 0; i < 16; i++)
    {
        //if (i == 0b0101 || i == 0b1000 || i == 0b1100)
        if (_excluded_indexes.find(i) != _excluded_indexes.cend())
            continue;
        auto dist = Dist(_palette_tc[i], point);
        if (dist < min_dist)
        {
            res = i;
            min_dist = dist;
        }
    }
    return DistanceInfo{ res, min_dist };

}

void Nearest::EnsureTC()
{
    for (unsigned i = 0; i < 16; i++)
    {
        _expanded_pal[i] = Expand(_palette[i]);
    }
}
