#include "Mvn.h"

using namespace std;

Mvn::Mvn() {


}

Mvn::Mvn(const Eigen::VectorXd& mu, const Eigen::MatrixXd& s) {

	mean = mu;
	sigma = s;

}

double Mvn::pdf(const Eigen::VectorXd& x) const
{
	double n = x.rows();
	double sqrt2pi = sqrt(2 * M_PI);
	double quadform = (x - mean).transpose() * sigma.inverse() * (x - mean);
	double norm = pow(sqrt2pi, -n) *
		pow(sigma.determinant(), -0.5);
	return norm * exp(-0.5 * quadform);
}

Eigen::VectorXd Mvn::morphMean(vector<double> mean) {

	Eigen::VectorXd mean_e(3);

	mean_e << mean[0], mean[1], mean[2];

	return mean_e;

}

Eigen::MatrixXd Mvn::morphSigma(array<array<double, 3>, 3> covarianceMatrix) {

	Eigen::MatrixXd sigma(3, 3);

	sigma << covarianceMatrix[0][0], covarianceMatrix[0][1], covarianceMatrix[0][2],
			 covarianceMatrix[1][0], covarianceMatrix[1][1], covarianceMatrix[1][2],
			 covarianceMatrix[2][0], covarianceMatrix[2][1], covarianceMatrix[2][2];

	return sigma;

}