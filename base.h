#pragma once
#include <optional>

#define _CRT_SECURE_NO_WARNINGS 1
#include <opencv2/core/utility.hpp>
#define SQR(x) ((x)*(x))

struct RGB
{
    uchar r{ 0 };
    uchar g{ 0 };
    uchar b{ 0 };
};

struct ColorInfo
{
    uchar rgb256;
    cv::Vec3b tc;
};

struct DistanceInfo
{
    unsigned indx; // index of closes entry
    int dinstance;
};


struct GlobalStat
{
    std::optional<unsigned> col_global0_indx;
    std::optional<unsigned> col_global1_indx;
    std::optional<unsigned> col_global0;
    std::optional<unsigned> col_global1;
    std::optional<RGB> col_global0_rgb;
    std::optional<RGB> col_global1_rgb;
};

bool operator<(const RGB& i, const RGB& o);
uchar ExpandRG(uchar in);
uchar ExpandB(uchar in);
cv::Vec3b Expand(unsigned); // expands ULA+ 
cv::Vec3b Expand(const RGB& cp);
RGB ToRGB(const cv::Vec3b&);
RGB ToRGBf(const cv::Vec3b&);
uchar Pack(const RGB& c);

// distance of TrueColor entries
int Dist(const cv::Vec3b& a, const cv::Vec3b& b);
int Dist(const RGB& a, const cv::Vec3b& b);
int Dist(unsigned a0, const cv::Vec3b& b);

// nearest on of 'psize' entries in palete
// return entry, distance
DistanceInfo NearestPal(
    const std::vector<RGB>& pal,
    unsigned start,
    unsigned psize,
    const cv::Vec3b& point);

//palettes
extern uchar spectrum_natives[16];
extern uchar spectrum_more_red[16];
extern uchar spectrum_more_green[16];
extern uchar spectrum_more_green2[16];
extern uchar spectrum_more_rg[16];
extern uchar spectrum_DB16[16];
extern cv::Vec3b DB16_TC[16];

