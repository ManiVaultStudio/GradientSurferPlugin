#pragma once

#include <vector>
#include <Eigen/Dense>
#include <Eigen/SVD>
#include <stdexcept>
#include <QtConcurrent>
#include <functional>
#include <string>
#include <optional>
#include <QElapsedTimer>
#include <QDebug>
#include <algorithm>
#include <numeric>

// Spectra headers for large-scale PCA
#include <Spectra/SymEigsSolver.h>
#include <Spectra/MatOp/DenseSymMatProd.h>

class PcaFast {
public:
    // Normalization strategies
    enum class DataNorm {
        NONE,      // no normalization
        MEAN,      // mean normalization
        MINMAX     // min-max normalization
    };

    // PCA algorithms
    enum class PcaAlgorithm {
        SVD,       // Use singular value decomposition
        COV        // Compute eigenvalues of covariance matrix
    };

    void setCompletionCallback(std::function<void(bool, const std::string&)> cb) {
        completionCallback_ = std::move(cb);
    }

    void fit(const std::vector<float>& data, int num_rows, int num_dims, int n_components = -1,
        PcaAlgorithm algorithm = PcaAlgorithm::SVD,
        DataNorm norm = DataNorm::NONE,
        bool enforce_standard_orientation = true) {
        QElapsedTimer timer;
        timer.start();

        bool success = true;
        std::string message = "PCA completed successfully";

        try {
            // Validate input
            if (data.size() != static_cast<size_t>(num_rows * num_dims)) {
                throw std::runtime_error("Data size does not match num_rows * num_dims");
            }

            // Convert to Eigen matrix (row-major)
            Eigen::Map<const Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>
                data_matrix(data.data(), num_rows, num_dims);

            // --- 1. Normalize first ---
            Eigen::MatrixXf normalized_data;
            switch (norm) {
            case DataNorm::MINMAX:
                normalized_data = minMaxNormalization(data_matrix);
                break;
            case DataNorm::MEAN:
                normalized_data = meanNormalization(data_matrix);
                break;
            case DataNorm::NONE:
            default:
                normalized_data = data_matrix;
                break;
            }

            // --- 2. Then center (subtract mean) ---
            mean_ = normalized_data.colwise().mean();
            Eigen::MatrixXf centered_data = normalized_data.rowwise() - mean_;

            // Determine number of components (match reference logic)
            int min_dim = std::min(num_rows, num_dims);
            int actual_n_components = n_components;
            if (actual_n_components > min_dim || actual_n_components <= 0) {
                actual_n_components = min_dim;
            }

            // --- 3. Perform PCA using selected algorithm ---
            Eigen::MatrixXf principal_components;
            if (algorithm == PcaAlgorithm::SVD) {
                principal_components = computePcaSvd(centered_data, actual_n_components);
            }
            else {
                principal_components = computePcaCov(centered_data, actual_n_components);
            }

            // --- 4. Project data ---
            Eigen::MatrixXf transformed_data = centered_data * principal_components;

            // --- 5. Apply standard orientation to transformed_data only ---
            if (enforce_standard_orientation) {
                transformed_data = standardOrientation(transformed_data);
            }

            // --- 6. Store results in row-major order ---
            scores_.resize(transformed_data.size());
            Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                scores_.data(), transformed_data.rows(), transformed_data.cols()) = transformed_data;

            loadings_.resize(principal_components.size());
            Eigen::Map<Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(
                loadings_.data(), principal_components.rows(), principal_components.cols()) = principal_components;

            // Store dimensions
            num_rows_ = num_rows;
            num_dims_ = num_dims;
            num_components_ = actual_n_components;

        }
        catch (const std::exception& ex) {
            success = false;
            message = ex.what();
            clearResults();
        }

        qDebug() << "PCA fit completed in" << timer.elapsed() << "ms";

        if (completionCallback_) {
            completionCallback_(success, message);
        }
    }

    // Getters
    const std::vector<float>& scores() const { return scores_; }
    const std::vector<float>& loadings() const { return loadings_; }
    const std::vector<float>& explained_variance() const { return explained_variance_; }
    const std::vector<float> mean() const {
        return std::vector<float>(mean_.data(), mean_.data() + mean_.size());
    }

    int numDims() const { return num_dims_; }
    int numComponents() const { return num_components_; }
    int numRows() const { return num_rows_; }

    std::optional<float> scoreAt(int row, int component) const {
        if (row < 0 || row >= num_rows_ || component < 0 || component >= num_components_)
            return std::nullopt;
        return scores_[row * num_components_ + component];
    }

    std::optional<float> loadingAt(int feature, int component) const {
        if (feature < 0 || feature >= num_dims_ || component < 0 || component >= num_components_)
            return std::nullopt;
        return loadings_[feature * num_components_ + component];
    }

private:
    void clearResults() {
        scores_.clear();
        loadings_.clear();
        explained_variance_.clear();
        num_rows_ = 0;
        num_dims_ = 0;
        num_components_ = 0;
    }

    Eigen::MatrixXf computePcaSvd(const Eigen::MatrixXf& centered_data, int n_components) {
        //qDebug() << "Computing PCA using SVD...";
        Eigen::BDCSVD<Eigen::MatrixXf> svd(centered_data, Eigen::ComputeThinV);

        if (svd.info() != Eigen::Success) {
            throw std::runtime_error("SVD computation failed");
        }

        // Use leftCols(n_components) for compatibility with older Eigen
        return svd.matrixV().leftCols(n_components);
    }

    Eigen::MatrixXf computePcaCov(const Eigen::MatrixXf& centered_data, int n_components) {
        //qDebug() << "Computing PCA using covariance matrix...";

        Eigen::MatrixXf cov_matrix = centered_data.transpose() * centered_data;
        Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf> solver(cov_matrix);

        if (solver.info() != Eigen::Success) {
            throw std::runtime_error("Eigen decomposition failed");
        }

        // Sort eigenvalues and eigenvectors in descending order
        Eigen::VectorXf eigenvalues = solver.eigenvalues();
        Eigen::MatrixXf eigenvectors = solver.eigenvectors();

        std::vector<int> idx(eigenvalues.size());
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(),
            [&eigenvalues](int i1, int i2) { return eigenvalues[i1] > eigenvalues[i2]; });

        Eigen::MatrixXf sorted_eigenvectors(eigenvectors.rows(), n_components);
        for (int i = 0; i < n_components; ++i) {
            sorted_eigenvectors.col(i) = eigenvectors.col(idx[i]);
        }

        return sorted_eigenvectors;
    }

    void computeExplainedVariance(const Eigen::MatrixXf& centered_data,
        const Eigen::MatrixXf& components) {
        Eigen::MatrixXf transformed = centered_data * components;
        explained_variance_.resize(components.cols());

        for (int i = 0; i < components.cols(); ++i) {
            explained_variance_[i] = transformed.col(i).squaredNorm() / (centered_data.rows() - 1);
        }
    }

    static Eigen::MatrixXf meanNormalization(const Eigen::MatrixXf& mat) {
        Eigen::MatrixXf mat_norm = mat.rowwise() - mat.colwise().mean();
        Eigen::VectorXf norm_factors = mat.colwise().maxCoeff() - mat.colwise().minCoeff();

        for (int col = 0; col < mat_norm.cols(); ++col) {
            if (norm_factors[col] > std::numeric_limits<float>::epsilon()) {
                mat_norm.col(col) /= norm_factors[col];
            }
        }
        return mat_norm;
    }

    static Eigen::MatrixXf minMaxNormalization(const Eigen::MatrixXf& mat) {
        Eigen::VectorXf min_vals = mat.colwise().minCoeff();
        Eigen::VectorXf norm_factors = mat.colwise().maxCoeff() - min_vals;
        Eigen::MatrixXf mat_norm = mat.rowwise() - min_vals.transpose();

        for (int col = 0; col < mat_norm.cols(); ++col) {
            if (norm_factors[col] > std::numeric_limits<float>::epsilon()) {
                mat_norm.col(col) /= norm_factors[col];
            }
        }
        return mat_norm;
    }

    // --- 4. Standard orientation: only apply to transformed_data, not principal_components ---
    template<typename Derived>
    static Eigen::Matrix<typename Derived::Scalar, Eigen::Dynamic, Eigen::Dynamic>
        standardOrientation(const Eigen::MatrixBase<Derived>& mat) {
        using Scalar = typename Derived::Scalar;
        Eigen::Matrix<Scalar, Eigen::Dynamic, Eigen::Dynamic> result = mat;

        Eigen::VectorXf signs(result.cols());
        for (int col = 0; col < result.cols(); ++col) {
            Eigen::Index max_row;
            result.col(col).cwiseAbs().maxCoeff(&max_row);
            signs[col] = (result(max_row, col) >= Scalar(0)) ? Scalar(1) : Scalar(-1);
        }
        return result.array().rowwise() * signs.transpose().array();
    }

    // Member variables
    std::vector<float> scores_;
    std::vector<float> loadings_;
    std::vector<float> explained_variance_;
    Eigen::RowVectorXf mean_;
    std::function<void(bool, const std::string&)> completionCallback_;
    int num_rows_ = 0;
    int num_dims_ = 0;
    int num_components_ = 0;
};