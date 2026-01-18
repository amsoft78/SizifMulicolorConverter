#pragma once
#include "Saver.h"

class Saver4 :
    public Saver
{
public:
    Saver4(const uchar* avail_zx_palette_as_rgb);

    virtual GlobalStat AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in) override;

    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;

    virtual cv::Vec3b CodePixel(unsigned row, unsigned col,
        const cv::Vec3b& in,
        const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base) override;

    virtual unsigned RowsInGroup() const override
    {
        return 8;
    }
    virtual unsigned ColsInGroup() const override
    {
        return 8;
    }

    virtual unsigned ColsInAnalyzedGroup() const override
    {
        return 8;
    }

    virtual std::set<RGB> UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base,
        unsigned current_column) const override;

    virtual void SavePalette(std::ofstream& of, const std::vector<RGB> pal, const std::string& prefix, const RGB* others) const override;

private:
    GlobalStat _g;
};

