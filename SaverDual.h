#pragma once
#include "Saver4.h"

class SaverDual :
    public Saver4
{
public:
    SaverDual(const uchar* avail_zx_palette_as_rgb);
    
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
        return 128;
    }

    virtual void SavePaletteAsAtributes(std::ofstream& of,
        const std::vector<RGB>& attribs,
        const std::string& prefix) const override;

    virtual void SaveHeader(std::ofstream& of, const std::string& project) override;
    virtual void SaveCFile(std::ofstream& of, const std::string& project, const std::vector<RGB>& attribs);

private:
    virtual void PutPixel(unsigned row, unsigned col, unsigned val) override;
};

