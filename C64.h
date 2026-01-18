#pragma once
#include <opencv2/core/utility.hpp>


class C64
{
public:
	static cv::Mat ReadCommodoreFLI(const std::string& filename);
};

