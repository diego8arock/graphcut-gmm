#ifndef GCAPPLICATION_H
#define GCAPPLICATION_H

#include <iostream>
#include <set>
#include <opencv2\highgui.hpp>
#include <opencv2\core.hpp>
#include <opencv2\imgproc.hpp>
#include <iomanip>
#include "Logger.h"
#include "Mvn.h"


class GCApplication
{

public:
	const cv::Scalar RED = cv::Scalar(0, 0, 255);
	const cv::Scalar PINK = cv::Scalar(230, 130, 255);
	const cv::Scalar BLUE = cv::Scalar(255, 0, 0);
	const cv::Scalar LIGHTBLUE = cv::Scalar(255, 255, 160);
	const cv::Scalar GREEN = cv::Scalar(0, 255, 0);
	const int BGD_KEY = cv::EVENT_FLAG_CTRLKEY;
	const int FGD_KEY = cv::EVENT_FLAG_SHIFTKEY;
	const int R_CHANNEL = 2;
	const int G_CHANNEL = 1;
	const int B_CHANNEL = 0;
	enum { NOT_SET = 0, IN_PROCESS = 1, SET = 2 };
	static const int radius = 2;
	static const int thickness = -1;
	void reset();
	void setImageAndWinName(const cv::Mat& _image, const std::string& _winName);
	void showImage() const;
	void mouseClick(int event, int x, int y, int flags, void* param);
	int nextIter();
	int getIterCount() const { return iterCount; }
	std::vector<cv::Point> getFgdPxls() { return fgdPxls; }
	Mvn getMvn(std::vector<cv::Point> seeds);
	void processSeeds();
	bool useMvn = false;

private:
	void setRectInMask();
	void setLblsInMask(int flags, cv::Point p, bool isPr);
	const std::string* winName;
	const cv::Mat* image;
	cv::Mat mvn_mask;
	cv::Mat mvn_result;
	cv::Mat mask;
	cv::Mat bgdModel, fgdModel;
	uchar rectState, lblsState, prLblsState;
	bool isInitialized;
	cv::Rect rect;
	std::vector<cv::Point> fgdPxls, bgdPxls, prFgdPxls, prBgdPxls;
	int iterCount;
	std::vector<double> calculateMean(std::vector<cv::Vec3b> pixels);
	std::vector<double> calculateVariance(std::vector<cv::Vec3b> pixels);
	std::array<std::array<double, 3>, 3> calculateCovarianceMatrix(std::vector<cv::Vec3b> pixels, std::vector<double> mean);
	double calculateCovariance(std::vector<cv::Vec3b> pixels, int channel1, int channel2, double mean1, double mean2);
};

#endif