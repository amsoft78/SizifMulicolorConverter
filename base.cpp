#include "base.h"

bool operator<(const RGB& i, const RGB& o)
{
    if (i.r < o.r)
        return true;
    if (i.r > o.r)
        return false;
    if (i.g < o.g)
        return true;
    if (i.g > o.g)
        return false;
    return (i.b < o.b);
}

bool operator==(const RGB& i, const RGB& o)
{
    return i.r == o.r
        && i.g == o.g
        && i.b == o.b;
}

uchar ExpandRG(uchar in)
{
    uchar out = in << 5;
    if (in != 0)
        out = out | 0x1F;
    return out;
}

uchar ExpandB(uchar in)
{
    uchar out = in << 6;
    if (in != 0)
        out = out | 0x3F;
    return out;
}

cv::Vec3b Expand(unsigned c)
{
    return cv::Vec3b {
        ExpandB(c & 0x03),
        ExpandRG((c & 0xE0) >> 5),
        ExpandRG((c & 0x1C) >> 2)
    };
}
cv::Vec3b Expand(const RGB& cp)
{
    return cv::Vec3b {
        ExpandB(cp.b),
        ExpandRG(cp.g),
        ExpandRG(cp.r) };
}

RGB ToRGB(const cv::Vec3b& p)
{
    return ToRGBf(p);
    // check only older 3 bits
    RGB col;
    col.b = p[0] >> 6;
    col.g = p[1] >> 5;
    col.r = p[2] >> 5;
    return col;
}

RGB ToRGBf(const cv::Vec3b& p)
{
    RGB col;
    /*col.b = p[0] >> 6;
    col.g = p[1] >> 5;
    col.r = p[2] >> 5;
    if (col.b == 0 && p[0] > 47)
        col.b = 1;
    if (col.g == 0 && p[1] > 23)
        col.g = 1;
    if (col.r == 0 && p[2] > 23)
        col.r = 1;
        */
    auto b = p[0] >> 6;
    auto g = p[1] >> 5;
    auto r = p[2] >> 5;
    auto bf = p[0] / 64.0 - b;
    auto gf = p[1] / 32.0 - g;
    auto rf = p[2] / 32.0 - r;
    if (b < 3 && bf >= 0.8)
        b++;
    if (g < 7 && gf >= 0.75)
        g++;
    if (r < 7 && rf >= 0.75)
        r++;
    col.b = b;
    col.g = g;
    col.r = r;
    return col;
}

uchar Pack(const RGB& c)
{
    uchar uc = (c.g << 5)
        | (c.r << 2)
        | (c.b);

    return uc;
}

RGB Unpack(uchar c)
{
    RGB rgb;
    rgb.b = c & 0x03;
    rgb.g = (c & 0xE0) >> 5;
    rgb.r = (c & 0x1C) >> 2;
    
    return rgb;
}

int Dist(const RGB& a, const cv::Vec3b& b)
{
    cv::Vec3b a_tc{
        ExpandB(a.b),
        ExpandRG(a.g),
        ExpandRG(a.r)
    };
    return Dist(a_tc, b);
}

// helpers
void RGBToHSV(const unsigned r, const unsigned g, const unsigned b, double& h, double& s, double& v)
{
    unsigned temp_min = std::min(r, std::min(g, b));
    v = std::max(r, std::max(g, b));
    if (temp_min == v)
    {
        h = 0;
    }
    else
    {
        if (r == v)
            h = 0.0 + (((int)g - (int)b) * 60.0 / (v - temp_min));
        if (g == v)
            h = 120.0 + (((int)b - (int)r) * 60.0 / (v - temp_min));
        if (b == v)
            h = 240.0 + (((int)r - (int)g) * 60.0 / (v - temp_min));
    }
    if (h < 0)
        h = h + 360;
    // s
    if (v == 0)
        s = 0;
    else
        s = (v - temp_min) * 100.0 / v;

    // v
    v = (100.0 * v) / 255.0;

}

// distance of TrueColor entries
int Dist(const cv::Vec3b& a, const cv::Vec3b& b)
{
    uchar a_blue = a[0];
    uchar a_green = a[1];
    uchar a_red = a[2];
    uchar b_blue = b[0];
    uchar b_green = b[1];
    uchar b_red = b[2];
    //auto res = abs(b_blue - a_blue) + abs(b_green - a_green) + abs (b_red - a_red);
    auto res = SQR(b_blue - a_blue) + SQR(b_green - a_green) + SQR(b_red - a_red);
    //return res;

    auto Ya = 0.299 * a_red + 0.587 * a_green + 0.114 * a_blue;
    auto Yb = 0.299 * b_red + 0.587 * b_green + 0.114 * b_blue;
    auto Ua = 0.492 * (a_blue - Ya);
    auto Ub = 0.492 * (b_blue - Yb);
    auto Va = 0.877 * (a_red - Ya);
    auto Vb = 0.877 * (b_red - Yb);

    auto y_delta = SQR(Ya - Yb);
    auto u_delta = SQR(Ua - Ub);
    auto v_delta = SQR(Va - Vb);

    res = std::max(u_delta, v_delta) * 1.5 + std::min(u_delta, v_delta) * 0.5;
    //return res + y_delta / 1.5;
    return res +y_delta / 1.75;

    double ha, sa, va;
    double hb, sb, vb;
    RGBToHSV(a_red, a_green, a_blue, ha, sa, va);
    RGBToHSV(b_red, b_green, b_blue, hb, sb, vb);

    auto res2 = abs(ha - hb) + abs (sa-sb) / 100 + abs (va-vb)/ 100;
    if (res2 == 0 && res != 0)
        printf("");
    return res2;

}

// distance of ULA+ entry
int Dist(unsigned a0, const cv::Vec3b& b)
{
    RGB a{
        (a0 >> 2) & 0x07,
        (a0 >> 5) & 0x07,
        a0 & 0x03
    };
    return Dist(a, b);
}

// nearest on of 'psize' entries in palete
// return entry, distance
DistanceInfo  NearestPal(
    const std::vector<RGB>& pal,
    unsigned start,
    unsigned psize,
    const cv::Vec3b& point)
{
    unsigned res = 0;
    int min_dist = Dist(pal[start], point);
    for (int i = 1; i < psize; i++)
    {
        auto dist = Dist(pal[start + i], point);
        if (dist < min_dist)
        {
            res = i;
            min_dist = dist;
        }
    }
    return DistanceInfo{ res, min_dist };
}

uchar spectrum_more_rg[16] = // 6080 (010 100) more R & G
{
    0b00000000,     // 0000 black,
    0b00000010,     // 0001 blue
    0b00010000,     // 0010 red
    0b00010010,     // 0011 magenta.
    0b10000000,     // 0100 green 
    0b10000010,     // 0101 cyan
    0b10010000,     // 0110 yellow
    0b10010010,     // 0111 gray
   
    0b01011000,     // 1000 brown
    0b01011011,     // 1001
    0b01011100,     // 1010
    0b01011111,     // 1011
    0b11111000,     // 1100 
    0b11111011,     // 1101 
    0b11111100,     // 1110 yellow
    0b11111111,     // 1111 white
};