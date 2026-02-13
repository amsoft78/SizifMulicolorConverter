#pragma once
#include "Saver.h"

class SaverHR :
    public Saver
{
public:
    SaverHR();

    virtual GlobalStat AnalyzeGlobal(const cv::Vec3b* avail_paletteTC, const cv::Mat& in) override;

    virtual cv::Vec3b CodePixel(unsigned row, unsigned col,
        const cv::Vec3b& in,
        const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base) override;

    virtual unsigned RowsInGroup() const override
    {
        return 1;
    }
    virtual unsigned ColsInGroup() const override
    {
        return 1;
    }

    virtual unsigned ColsInAnalyzedGroup() const override
    {
        return 1;
    }

    virtual unsigned ScreenColumns() const override
    {
        return 512;
    }

    virtual std::set<RGB> UsePrevPaletteEntries(const std::vector<RGB>& pal_rgb,
        unsigned pal_indx_base,
        unsigned current_column,
        unsigned current_row) const override;

    virtual bool CanUseNativeZXEntry(unsigned) override
    {
        return false;// don't case
    }

    virtual void SaveHeader(std::ofstream& of, const std::string& project) override;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs) override;

protected:
    //GlobalStat _g;
    // rows with ULAPlus color 00
    //std::vector<RGB> _color0;
    // rows with ULAPlus color 4
    //std::vector<RGB> _color4;

    //void _SaveHeader(std::ofstream& of, const std::string& project) const;

    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;
};

