#pragma once
#include <Eigen/Dense>

// Operator for Spectra: computes y = (X^T X) x, without forming X^T X
class PcaFastOp
{
public:
    // Required typedefs for Spectra compatibility
    using Scalar = double;
    using RealScalar = double;

    // Xc: centered data, shape (n_samples, n_features)
    PcaFastOp(const Eigen::MatrixXf& Xc)
        : Xc_(Xc)
    {}

    int rows() const { return static_cast<int>(Xc_.cols()); }
    int cols() const { return static_cast<int>(Xc_.cols()); }

    // y_out = (Xc^T Xc) x_in
    void perform_op(const double* x_in, double* y_out) const
    {
        Eigen::Map<const Eigen::VectorXd> x(x_in, Xc_.cols());
        // temp = Xc * x
        Eigen::VectorXf temp = Xc_ * x.cast<float>();
        // y = Xc^T * temp
        Eigen::VectorXf y = Xc_.transpose() * temp;
        Eigen::Map<Eigen::VectorXd>(y_out, Xc_.cols()) = y.cast<double>();
    }

private:
    const Eigen::MatrixXf& Xc_;
};