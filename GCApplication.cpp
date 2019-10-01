#include "GCApplication.h"
#include <map>

using namespace cv;
using namespace std;
using namespace detail;

static void getBinMask(const Mat& comMask, Mat& binMask)
{
	if (comMask.empty() || comMask.type() != CV_8UC1)
		CV_Error(Error::StsBadArg, "comMask is empty or has incorrect type (not CV_8UC1)");
	if (binMask.empty() || binMask.rows != comMask.rows || binMask.cols != comMask.cols)
		binMask.create(comMask.size(), CV_8UC1);
	binMask = comMask & 1;
}

void GCApplication::reset()
{
	if (!mask.empty())
		mask.setTo(Scalar::all(GC_BGD));
	bgdPxls.clear(); fgdPxls.clear();
	prBgdPxls.clear();  prFgdPxls.clear();
	isInitialized = false;
	rectState = NOT_SET;
	lblsState = NOT_SET;
	prLblsState = NOT_SET;
	iterCount = 0;
}
void GCApplication::setImageAndWinName(const Mat& _image, const std::string& _winName)
{
	if (_image.empty() || _winName.empty())
		return;
	image = &_image;
	mvn_mask = Mat(image->rows, image->cols, CV_8UC3, Scalar(255,255,255));
	mvn_result = Mat(image->rows, image->cols, CV_8UC3, Scalar(255, 255, 255));
	winName = &_winName;
	mask.create(image->size(), CV_8UC1);
	reset();

	if(useMvn)
		rectState = SET;
}
void GCApplication::showImage() const
{
	if (image->empty() || winName->empty())
		return;
	Mat res;
	Mat binMask;
	if (!isInitialized)
		image->copyTo(res);
	else
	{
		getBinMask(mask, binMask);
		image->copyTo(res, binMask);
	}
	vector<Point>::const_iterator it;
	for (it = bgdPxls.begin(); it != bgdPxls.end(); ++it)
		circle(res, *it, radius, BLUE, thickness);
	for (it = fgdPxls.begin(); it != fgdPxls.end(); ++it)
		circle(res, *it, radius, RED, thickness);
	for (it = prBgdPxls.begin(); it != prBgdPxls.end(); ++it)
		circle(res, *it, radius, LIGHTBLUE, thickness);
	for (it = prFgdPxls.begin(); it != prFgdPxls.end(); ++it)
		circle(res, *it, radius, PINK, thickness);
	if (rectState == IN_PROCESS || rectState == SET)
		rectangle(res, Point(rect.x, rect.y), Point(rect.x + rect.width, rect.y + rect.height), GREEN, 2);
	imshow(*winName, res);
}
void GCApplication::setRectInMask()
{
	CV_Assert(!mask.empty());
	mask.setTo(GC_BGD);
	rect.x = max(0, rect.x);
	rect.y = max(0, rect.y);
	rect.width = min(rect.width, image->cols - rect.x);
	rect.height = min(rect.height, image->rows - rect.y);
	(mask(rect)).setTo(Scalar(GC_PR_FGD));
}
void GCApplication::setLblsInMask(int flags, Point p, bool isPr)
{
	vector<Point>* bpxls, * fpxls;
	uchar bvalue, fvalue;
	if (!isPr)
	{
		bpxls = &bgdPxls;
		fpxls = &fgdPxls;
		bvalue = GC_BGD;
		fvalue = GC_FGD;
	}
	else
	{
		bpxls = &prBgdPxls;
		fpxls = &prFgdPxls;
		bvalue = GC_PR_BGD;
		fvalue = GC_PR_FGD;
	}
	if (flags & BGD_KEY)
	{
		bpxls->push_back(p);
		circle(mask, p, radius, bvalue, thickness);
	}
	if (flags & FGD_KEY)
	{
		fpxls->push_back(p);
		circle(mask, p, radius, fvalue, thickness);
	}
}
void GCApplication::mouseClick(int event, int x, int y, int flags, void*)
{
	// TODO add bad args check
	switch (event)
	{
	case EVENT_LBUTTONDOWN: // set rect or GC_BGD(GC_FGD) labels
	{
		bool isb = (flags & BGD_KEY) != 0,
			isf = (flags & FGD_KEY) != 0;
		if (rectState == NOT_SET && !isb && !isf)
		{
			rectState = IN_PROCESS;
			rect = Rect(x, y, 1, 1);
		}
		if ((isb || isf) && rectState == SET)
			lblsState = IN_PROCESS;
	}
	break;
	case EVENT_RBUTTONDOWN: // set GC_PR_BGD(GC_PR_FGD) labels
	{
		bool isb = (flags & BGD_KEY) != 0,
			isf = (flags & FGD_KEY) != 0;
		if ((isb || isf) && rectState == SET)
			prLblsState = IN_PROCESS;
	}
	break;
	case EVENT_LBUTTONUP:
		if (rectState == IN_PROCESS)
		{
			rect = Rect(Point(rect.x, rect.y), Point(x, y));
			rectState = SET;
			setRectInMask();
			CV_Assert(bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty());
			showImage();
		}
		if (lblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), false);
			lblsState = SET;
			showImage();
		}
		break;
	case EVENT_RBUTTONUP:
		if (prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true);
			prLblsState = SET;
			showImage();
		}
		break;
	case EVENT_MOUSEMOVE:
		if (rectState == IN_PROCESS)
		{
			rect = Rect(Point(rect.x, rect.y), Point(x, y));
			CV_Assert(bgdPxls.empty() && fgdPxls.empty() && prBgdPxls.empty() && prFgdPxls.empty());
			showImage();
		}
		else if (lblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), false);
			showImage();
		}
		else if (prLblsState == IN_PROCESS)
		{
			setLblsInMask(flags, Point(x, y), true);
			showImage();
		}
		break;
	}
}
int GCApplication::nextIter()
{
	if (isInitialized)
		grabCut(*image, mask, rect, bgdModel, fgdModel, 1);
	else
	{
		if (rectState != SET)
			return iterCount;
		if (lblsState == SET || prLblsState == SET)
			grabCut(*image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_MASK);
		else
			grabCut(*image, mask, rect, bgdModel, fgdModel, 1, GC_INIT_WITH_RECT);
		isInitialized = true;
	}
	iterCount++;
	bgdPxls.clear(); fgdPxls.clear();
	prBgdPxls.clear(); prFgdPxls.clear();
	return iterCount;
}

struct lex_compare {
	bool operator() (const Vec3b lhs, const Vec3b rhs) const {
		return lhs[0] < rhs[0] || lhs[1] < rhs[1] || lhs[2] < rhs[2];
	}
};

bool double_equals(double a, double b, double epsilon = 0.001)
{
	return std::abs(a - b) < epsilon;
}

void GCApplication::processSeeds() {

	Mvn mvnBG, mvnFG;

	Logger::Instance()->writeToLog("Processing BG Pixels");
	mvnBG = getMvn(bgdPxls);

	Logger::Instance()->writeToLog("Processing FG Pixels");
	mvnFG = getMvn(fgdPxls);

	int rows = image->rows;
	int cols = image->cols;

	int i, j;
	int countBG = 0, countFG = 0;
	Vec3b black(0, 0, 0);
	cout << rows << endl;
	for (i = 0; i < rows; ++i)
	{
		for (j = 0; j < cols; ++j)
		{
			Vec3b value = (*image).at<Vec3b>(i, j);
			Eigen::VectorXd test(3);
			test << static_cast<double>(value[B_CHANNEL]), static_cast<double>(value[G_CHANNEL]), static_cast<double>(value[R_CHANNEL]);
			double result1 = mvnBG.pdf(test);
			double result2 = mvnFG.pdf(test);
			clog << "BG result: " << setprecision(30) << result1 << " FB result: " << result2 << endl;
			if (result1 < result2) {
				mvn_mask.at<Vec3b>(i, j) = black;
				countBG++;
			}
			else {
				countFG++;
			}

		}

		if (i % 100 == 0)
			cout << i << endl;
	}

	clog << countBG << endl;
	clog << countFG << endl;

	imwrite("mask_image.png", mvn_mask);
	imshow("mask", mvn_mask);

	for (i = 0; i < rows; ++i)
	{
		for (j = 0; j < cols; ++j)
		{
			if (mvn_mask.at<Vec3b>(i, j) == black) 
				mvn_result.at<Vec3b>(i, j) = image->at<Vec3b>(i, j);			
			else			
				mvn_result.at<Vec3b>(i, j) = black;			
		}
	}

	imwrite("result_image.png", mvn_result);
	imshow("result", mvn_result);	
}

Mvn GCApplication::getMvn(vector<Point> seeds) {

	char str[500];
	Logger::Instance()->writeToLog("Calculating Mean");

	set<Vec3b, lex_compare> setSeeds;

	for (Point p : seeds) {

		Vec3b value = (*image).at<Vec3b>(p);
		setSeeds.insert(value);

	}

	strcpy_s(str, "Total different pixels: ");
	strcat_s(str, std::to_string(setSeeds.size()).c_str());

	vector<Vec3b> uniqueSeeds;
	set< Vec3b, lex_compare>::iterator it = setSeeds.begin();
	while (it != setSeeds.end()) {

		auto v = (*it);
		Vec3b value(v[B_CHANNEL], v[G_CHANNEL], v[R_CHANNEL]);
		uniqueSeeds.push_back(value);
		it++;

	}

	Logger::Instance()->writeToLog(str);

	vector<double> mean = calculateMean(uniqueSeeds);

;	strcpy_s(str, "Mean B: ");
	strcat_s(str, std::to_string(mean[B_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);
	strcpy_s(str, "Mean G: ");
	strcat_s(str, std::to_string(mean[G_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);
	strcpy_s(str, "Mean R: ");
	strcat_s(str, std::to_string(mean[R_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);

	Logger::Instance()->writeToLog("Calculating Sigma");
	array<array<double, 3>, 3> sigma = calculateCovarianceMatrix(uniqueSeeds, mean);

	Logger::Instance()->writeToLog("Covaraince Matrix");
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)       
			clog << sigma[i][j] << " \n"[j == 3 - 1];
	
	Logger::Instance()->writeToLog("Moprhing Mean");
	Eigen::VectorXd mean_e = Mvn::morphMean(mean);
	
	Logger::Instance()->writeToLog("Morphing Sigma");
	Eigen::MatrixXd sigma_e = Mvn::morphSigma(sigma);
	
	Mvn mvn(mean_e, sigma_e);

	return mvn;

}

array<array<double, 3>, 3> GCApplication::calculateCovarianceMatrix(vector<Vec3b> pixels, vector<double> mean) {

	vector<double> variance = calculateVariance(pixels);
	array<array<double, 3>, 3> mtx;

	mtx[R_CHANNEL][R_CHANNEL] = variance[R_CHANNEL];
	mtx[G_CHANNEL][G_CHANNEL] = variance[G_CHANNEL];
	mtx[B_CHANNEL][B_CHANNEL] = variance[B_CHANNEL];

	char str[500];
	strcpy_s(str, "Variance B: ");
	strcat_s(str, std::to_string(variance[B_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);
	strcpy_s(str, "Variance G: ");
	strcat_s(str, std::to_string(variance[G_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);
	strcpy_s(str, "Variance R: ");
	strcat_s(str, std::to_string(variance[R_CHANNEL]).c_str());
	Logger::Instance()->writeToLog(str);

	mtx[B_CHANNEL][G_CHANNEL] = calculateCovariance(pixels, B_CHANNEL, G_CHANNEL, mean[B_CHANNEL], mean[G_CHANNEL]);
	mtx[B_CHANNEL][R_CHANNEL] = calculateCovariance(pixels, B_CHANNEL, R_CHANNEL, mean[B_CHANNEL], mean[R_CHANNEL]);
	mtx[G_CHANNEL][R_CHANNEL] = calculateCovariance(pixels, G_CHANNEL, R_CHANNEL, mean[G_CHANNEL], mean[R_CHANNEL]);

	mtx[G_CHANNEL][B_CHANNEL] = mtx[B_CHANNEL][G_CHANNEL];
	mtx[R_CHANNEL][B_CHANNEL] = mtx[B_CHANNEL][R_CHANNEL];
	mtx[R_CHANNEL][G_CHANNEL] = mtx[G_CHANNEL][R_CHANNEL];

	return mtx;

}

double GCApplication::calculateCovariance(vector<Vec3b> pixels, int channel1, int channel2, double mean1, double mean2) {

	double covariance = 0;

	for (Vec3b p : pixels) {

		Vec3b value = p;

		double c1 = static_cast<double>(value[channel1]);
		double c2 = static_cast<double>(value[channel2]);
		covariance += (c1 - mean1) * (c2 - mean2);
		
	}

	double size = static_cast<double>(pixels.size());
	return covariance / (size - 1);

}

vector<double> GCApplication::calculateVariance(vector<Vec3b> pixels) {

	vector<double> mean = calculateMean(pixels);

	double varianceR = 0;
	double varianceG = 0;
	double varianceB = 0;

	for (Vec3b p : pixels) {
		
		Vec3b value = p;

		varianceR += pow(static_cast<double>(value[R_CHANNEL]) - mean[R_CHANNEL], 2);
		varianceG += pow(static_cast<double>(value[G_CHANNEL]) - mean[G_CHANNEL], 2);
		varianceB += pow(static_cast<double>(value[B_CHANNEL]) - mean[B_CHANNEL], 2);
	
	}

	double size = static_cast<double>(pixels.size());
	varianceR /= size - 1;
	varianceG /= size - 1;
	varianceB /= size-  1;

	vector<double> v(3);
	v[R_CHANNEL] = varianceR;
	v[G_CHANNEL] = varianceG;
	v[B_CHANNEL] = varianceB;

	return v;

}

vector<double> GCApplication::calculateMean(vector<Vec3b> pixels) {

	double meanR = 0;
	double meanG = 0;
	double meanB = 0;

	for (Vec3b p : pixels) {

		Vec3b value = p;

		meanR += static_cast<double>(value[R_CHANNEL]);
		meanG += static_cast<double>(value[G_CHANNEL]);
		meanB += static_cast<double>(value[B_CHANNEL]);	

	}

	double size = static_cast<double>(pixels.size());
	meanR /= size;
	meanG /= size;
	meanB /= size;

	vector<double> v(3);
	v[R_CHANNEL] = meanR;
	v[G_CHANNEL] = meanG;
	v[B_CHANNEL] = meanB;

	return v;

}

