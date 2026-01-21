#pragma once

#include "Saver.h"
#include <memory>

class Nearest;

class Saver16 :
    public Saver
{
public:

    Saver16(const uchar* avail_zx_palette_as_rgb);
    virtual ~Saver16();

    virtual GlobalStat AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in) override;

    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;
   
    virtual cv::Vec3b CodePixel(unsigned row, unsigned col,
        const cv::Vec3b& p,
        const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base) override;

    virtual unsigned RowsInGroup() const override
    {
        return 8;
    }
    virtual unsigned ColsInGroup() const override
    {
        return 4;
    }

    // when analyzing, check wide group which will be effected by current paletee entries
    virtual unsigned ColsInAnalyzedGroup() const override
    {
        return 8;
    }

    virtual std::set<RGB> UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base,
        unsigned current_column) const override;

    virtual bool CanUseNativeZXEntry(unsigned) override;

    virtual void SaveHeader(std::ofstream& of, const std::string& project) override;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs) override;

private:
    std::unique_ptr<Nearest> _nearest12;
    unsigned _border_color{ 0 };
    RGB _border_rgb;
};

