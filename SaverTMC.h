#pragma once
#include "Saver.h"
#include <map>

class SaverTMC :
    public Saver
{
public:
    SaverTMC(const uchar* avail_zx_palette_as_rgb);

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
        return false;
    }

    virtual void SaveHeader(std::ofstream& of, const std::string& project) override;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs) override;

protected:
    GlobalStat _g;
    const bool _up{ true };
    std::vector<unsigned char> _ula_plus_palette;
    std::map<uint16_t, unsigned> _pair_code2chunk; // mapping of color pair into chunk

    // additional group color statistics (needed for ULA+ palette design)
    void AnalyzeWholeBuiltPalette(const std::vector<RGB>& pal_rgb);

    uint16_t GetByteAddressInPage(unsigned row, unsigned col) const;
    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;
    virtual std::string GetFullProjectName(const std::string& core) const;

    virtual void SavePaletteAsAtributes(std::ofstream& of,
        const std::vector<RGB>& attribs,
        const std::string& prefix) const override;

};

