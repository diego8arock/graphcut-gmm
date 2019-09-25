#include "Eigen/Dense"
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include <vector>
#include <array>

class Mvn
{
public:
	Mvn();
	Mvn(const Eigen::VectorXd& mu,
		const Eigen::MatrixXd& s);
	//~Mvn();
	double pdf(const Eigen::VectorXd& x) const;
	//Eigen::VectorXd sample(unsigned int nr_iterations = 20) const;
	Eigen::VectorXd mean;
	Eigen::MatrixXd sigma;
	static Eigen::VectorXd morphMean(std::vector<double> mean);
	static Eigen::MatrixXd morphSigma(std::array<std::array<double, 3>, 3> covarianceMatrix);
};
