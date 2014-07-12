#include "aslam/cameras/fisheye-distortion.h"

namespace aslam {

bool FisheyeDistortion::operator==(
    const aslam::Distortion& other) const {
  const aslam::Distortion* other_fisheye_distortion =
      dynamic_cast<const aslam::Distortion*>(&other);
  if (other_fisheye_distortion) {
    Eigen::VectorXd other_parameters;
    other_fisheye_distortion->getParameters(&other_parameters);
    if (this->params_ == other_parameters) {
      return true;
    }
  }
  return false;
}

void FisheyeDistortion::distort(
    Eigen::Matrix<double, 2, 1>* point) const {
  CHECK_NOTNULL(point);
  distort(point, NULL);
}

void FisheyeDistortion::distort(const Eigen::Matrix<double, 2, 1>& point,
                                Eigen::Matrix<double, 2, 1>* out_point) const {
  CHECK_NOTNULL(out_point);
  *out_point = point;
  distort(out_point, NULL);
}

void FisheyeDistortion::distort(
    Eigen::Matrix<double, 2, 1>* point,
    Eigen::Matrix<double, 2, Eigen::Dynamic>* out_jacobian) const {
  CHECK_NOTNULL(point);

  const double& w = params_(0);
  const double r_u = point->norm();
  const double r_u_cubed = r_u * r_u * r_u;
  const double tanwhalf = tan(w / 2.);
  const double tanwhalfsq = tanwhalf * tanwhalf;
  const double atan_wrd = atan(2. * tanwhalf * r_u);
  double r_rd;

  if (w * w < 1e-5) {
    // Limit w > 0.
    r_rd = 1.0;
  } else {
    if (r_u * r_u < 1e-5) {
      // Limit r_u > 0.
      r_rd = 2. * tanwhalf / w;
    } else {
      r_rd = atan_wrd / (r_u * w);
    }
  }

  const double& u = (*point)(0);
  const double& v = (*point)(1);

  // If Jacobian calculation is requested.
  if (out_jacobian) {
    out_jacobian->resize(2, 2);
    if (w * w < 1e-5) {
      out_jacobian->setIdentity();
    }
    else if (r_u * r_u < 1e-5) {
      out_jacobian->setIdentity();
      // The coordinates get multiplied by an expression not depending on r_u.
      *out_jacobian *= (2. * tanwhalf / w);
    }
    else {
      const double duf_du = (atan_wrd) / (w * r_u)
                - (u * u * atan_wrd) / (w * r_u_cubed)
                + (2 * u * u * tanwhalf)
                / (w * (u * u + v * v) * (4 * tanwhalfsq * (u * u + v * v) + 1));
      const double duf_dv = (2 * u * v * tanwhalf)
                / (w * (u * u + v * v) * (4 * tanwhalfsq * (u * u + v * v) + 1))
                - (u * v * atan_wrd) / (w * r_u_cubed);
      const double dvf_du = (2 * u * v * tanwhalf)
                / (w * (u * u + v * v) * (4 * tanwhalfsq * (u * u + v * v) + 1))
                - (u * v * atan_wrd) / (w * r_u_cubed);
      const double dvf_dv = (atan_wrd) / (w * r_u)
                - (v * v * atan_wrd) / (w * r_u_cubed)
                + (2 * v * v * tanwhalf)
                / (w * (u * u + v * v) * (4 * tanwhalfsq * (u * u + v * v) + 1));

      *out_jacobian <<
          duf_du, duf_dv,
          dvf_du, dvf_dv;
    }
  }

  *point *= r_rd;
}

void FisheyeDistortion::undistort(Eigen::Matrix<double, 2, 1>* point) const {
  CHECK_NOTNULL(point);

  const double& w = params_(0);
  double mul2tanwby2 = tan(w / 2.0) * 2.0;

  // Calculate distance from point to center.
  double r_d = point->norm();

  // Calculate undistorted radius of point.
  double r_u;
  if (fabs(r_d * w) <= kMaxValidAngle) {
    r_u = tan(r_d * w) / (r_d * mul2tanwby2);
  } else {
    r_u = std::numeric_limits<double>::infinity();
  }

  (*point) *= r_u;
}

// Passing NULL as *out_jacobian is admissible and makes the routine
// skip Jacobian calculation.
void FisheyeDistortion::distortParameterJacobian(
    const Eigen::Matrix<double, 2, 1>& point,
    Eigen::Matrix<double, 2, Eigen::Dynamic>* out_jacobian) const {
  CHECK_NOTNULL(out_jacobian);
  CHECK_EQ(out_jacobian->cols(), 1);

  const double& w = params_(0);

  const double tanwhalf = tan(w / 2.);
  const double tanwhalfsq = tanwhalf * tanwhalf;
  const double r_u = point.norm();
  const double atan_wrd = atan(2. * tanwhalf * r_u);

  const double& u = point(0);
  const double& v = point(1);

  if (w * w < 1e-5) {
    out_jacobian->setZero();
  }
  else if (r_u * r_u < 1e-5) {
    out_jacobian->setOnes();
    *out_jacobian *= (w - sin(w)) / (w * w * cos(w / 2) * cos(w / 2));
  }
  else {
    const double dxd_d_w = (2 * u * (tanwhalfsq / 2 + 0.5))
          / (w * (4 * tanwhalfsq * r_u * r_u + 1))
          - (u * atan_wrd) / (w * w * r_u);

    const double dyd_d_w = (2 * v * (tanwhalfsq / 2 + 0.5))
          / (w * (4 * tanwhalfsq * r_u * r_u + 1))
          - (v * atan_wrd) / (w * w * r_u);

    *out_jacobian << dxd_d_w, dyd_d_w;
  }
}

} // namespace aslam
