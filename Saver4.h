#pragma once
#include "Saver.h"

class Saver4 :
    public Saver
{
public:
    Saver4(const uchar* avail_zx_palette_as_rgb);

    virtual GlobalStat AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in) override;

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

    virtual unsigned ScreenColumns() const override
    {
        return 256;
    }

    virtual std::set<RGB> UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base,
        unsigned current_column,
        unsigned current_row) const override;

    virtual bool CanUseNativeZXEntry(unsigned) override
    {
        return false; /// in CGA4 no 16 colors palette entries are used directly
    }

    virtual void SaveHeader(std::ofstream& of, const std::string& project) override;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs) override;

protected:
    GlobalStat _g;
    // rows with ULAPlus color 00
    std::vector<RGB> _color0;
    // rows with ULAPlus color 4
    std::vector<RGB> _color4;

    void _SaveHeader(std::ofstream& of, const std::string& project) const;
    void _Save4thColor(std::ofstream& of, const std::string& project) const;
    void _Save0thColor(std::ofstream& of, const std::string& project) const;

    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;
};

