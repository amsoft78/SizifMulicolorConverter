#pragma once
#include "base.h"
#include <set>

class Saver
{
public:
    Saver(const uchar* avail_zx_palette_as_rgb);
    virtual ~Saver();
    virtual GlobalStat AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in) = 0;

    virtual cv::Vec3b CodePixel(unsigned row, unsigned col, const cv::Vec3b& p, const std::vector<RGB>& pal_rgb, unsigned pal_indx_base) = 0;
    virtual unsigned RowsInGroup() const = 0;
    virtual unsigned ColsInGroup() const = 0;
    virtual unsigned ColsInAnalyzedGroup() const = 0; // number of pixels to be analyzed can be bigger than group width

    virtual std::set<RGB> UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base,
        unsigned current_column) const = 0;

    virtual void SaveHeader(std::ofstream& of, const std::string& project) = 0;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs);

protected:
    uchar* out_page0;
    uchar* out_page1;
    uchar _zx_palette[16];

    virtual void PutPixel(unsigned row, unsigned col, unsigned val) = 0;
    virtual void SavePaletteAsAtributes(std::ofstream& of,
        const std::vector<RGB>& attribs,
        const std::string& prefix) const;
    void Save(std::ofstream& of, unsigned page) const;

    void SaveScreenPage(std::ofstream& of, unsigned page) const;
    
    //virtual void SavePalette(std::ofstream& of, const std::vector<RGB> pal, const std::string& prefix, const RGB* others) const = 0;
};



