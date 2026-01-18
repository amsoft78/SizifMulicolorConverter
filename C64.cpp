#include "base.h"
#define _CRT_SECURE_NO_WARNINGS 1
#include "C64.h"

#include "opencv2/imgproc.hpp"

static cv::Vec3b GetC64Color(uint8_t col)
{
    switch (col)
    {
    case 1:
        return cv::Vec3b{ 0xff, 0xff, 0xff };
    case 2:
        return cv::Vec3b{ 0x00, 0x00, 0x88 };
    case 3:
        return cv::Vec3b{ 0xee, 0xff, 0xaa };
    case 4:
        return cv::Vec3b{ 0xcc, 0x44, 0xcc };
    case 5:
        return cv::Vec3b{ 0x55, 0xcc, 0x00 };
    case 6:
        return cv::Vec3b{ 0xaa, 0x00, 0x00 };
    case 7:
        return cv::Vec3b{ 0x77, 0xee, 0xee };
    case 8:
        return cv::Vec3b{ 0x55, 0x88, 0xdd };
    case 9:
        return cv::Vec3b{ 0x00, 0x44, 0x66 };
    case 10:
        return cv::Vec3b{ 0x77, 0x77, 0xff };
    case 11:
        return cv::Vec3b{ 0x33, 0x33, 0x33 };
    case 12:
        return cv::Vec3b{ 0x77, 0x77, 0x77 };
    case 13:
        return cv::Vec3b{ 0x66, 0xff, 0xaa };
    case 14:
        return cv::Vec3b{ 0xff, 0x88, 0x00 };
    case 15:
        return cv::Vec3b{ 0xbb, 0xbb, 0xbb };
    default:
        return cv::Vec3b{ 0, 0, 0 };
    }
}

static cv::Vec3b GetRGB(uint8_t b, uint8_t color3rd, uint8_t colors12)
{
    if (b == 0)
        return cv::Vec3b(0, 0, 0);
    if (b == 3)
        return GetC64Color(color3rd);
    if (b == 1)
        return GetC64Color(colors12 >> 4);
    else
        return GetC64Color(colors12 & 0x0F);
}

cv::Mat C64::ReadCommodoreFLI(const std::string& filename)
{
    int bitmap_start = 8000;
    int skip_columns = 3;
    if (filename.find(".fli") != std::string::npos)
        bitmap_start = 8191;
    FILE* f = fopen(filename.c_str(), "rb");
    fseek(f, -bitmap_start, SEEK_END);
    auto buff = std::make_unique< std::array<uint8_t, 8000>>();
    fread(std::addressof(*buff), 1, 8000, f);
    fseek(f, -(bitmap_start + 8192 + 1024), SEEK_END);
    uint8_t colors[1000];
    fread(colors, 1, 1000, f);

    auto color_attr = std::make_unique< std::array<uint8_t, 8168>>();
    fseek(f, -(bitmap_start + 8192), SEEK_END);
    fread(std::addressof (*color_attr), 1, 8168, f);

    fclose(f);

    cv::Mat out(200, 160 - skip_columns * 4, CV_8UC3, cv::Scalar(0, 0, 0));

    for (int row = 0; row < 200; row++)
    {
        for (int x = skip_columns; x < 40; x++)
        {
            int col_indx = (row >> 3) * 40 + x;
            auto color3rd = colors[col_indx];
            auto colors12 = (*color_attr)[1024 * (row & 0x7) + col_indx];

            auto bL = (*buff)[(row >> 3) * 320 + (x << 3) + (row & 0x07)];

            uint8_t b0 = (bL & 0xC0) >> 6;
            uint8_t b1 = (bL & 0x30) >> 4;
            uint8_t b2 = (bL & 0x0C) >> 2;
            uint8_t b3 = bL & 0x03;

            out.at<cv::Vec3b>(row, (x - skip_columns) * 4) = GetRGB(b0, color3rd, colors12);
            out.at<cv::Vec3b>(row, (x - skip_columns) * 4 + 1) = GetRGB(b1, color3rd, colors12);
            out.at<cv::Vec3b>(row, (x - skip_columns) * 4 + 2) = GetRGB(b2, color3rd, colors12);
            out.at<cv::Vec3b>(row, (x - skip_columns) * 4 + 3) = GetRGB(b3, color3rd, colors12);

        }
    }

    return out;
}

