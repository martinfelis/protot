#ifndef _SIMPLEMATHGL_H_
#define _SIMPLEMATHGL_H_

#include "SimpleMath.h"
#include <cmath>

namespace SimpleMath {

typedef SimpleMath::Fixed::Matrix<float, 3, 1> Vector3f;
typedef SimpleMath::Fixed::Matrix<float, 3, 3> Matrix33f;

typedef SimpleMath::Fixed::Matrix<float, 4, 1> Vector4f;
typedef SimpleMath::Fixed::Matrix<float, 4, 4> Matrix44f;

namespace GL {

inline Matrix33f RotateMat33 (float rot_deg, float x, float y, float z) {
	float c = cosf (rot_deg * M_PI / 180.f);
	float s = sinf (rot_deg * M_PI / 180.f);
	return Matrix33f (
			x * x * (1.0f - c) + c,
			y * x * (1.0f - c) + z * s,
			x * z * (1.0f - c) - y * s,

			x * y * (1.0f - c) - z * s,
			y * y * (1.0f - c) + c,
			y * z * (1.0f - c) + x * s,

			x * z * (1.0f - c) + y * s,
			y * z * (1.0f - c) - x * s,
			z * z * (1.0f - c) + c

			);
}


inline Matrix44f RotateMat44 (float rot_deg, float x, float y, float z) {
	float c = cosf (rot_deg * M_PI / 180.f);
	float s = sinf (rot_deg * M_PI / 180.f);
	return Matrix44f (
			x * x * (1.0f - c) + c,
			y * x * (1.0f - c) + z * s,
			x * z * (1.0f - c) - y * s,
			0.f, 

			x * y * (1.0f - c) - z * s,
			y * y * (1.0f - c) + c,
			y * z * (1.0f - c) + x * s,
			0.f,

			x * z * (1.0f - c) + y * s,
			y * z * (1.0f - c) - x * s,
			z * z * (1.0f - c) + c,
			0.f,

			0.f, 0.f, 0.f, 1.f
			);
}

inline Matrix44f TranslateMat44 (float x, float y, float z) {
	return Matrix44f (
			1.f, 0.f, 0.f, 0.f,
			0.f, 1.f, 0.f, 0.f,
			0.f, 0.f, 1.f, 0.f,
			  x,   y,   z, 1.f
			);
}

inline Matrix44f ScaleMat44 (float x, float y, float z) {
	return Matrix44f (
			  x, 0.f, 0.f, 0.f,
			0.f,   y, 0.f, 0.f,
			0.f, 0.f,   z, 0.f,
			0.f, 0.f, 0.f, 1.f
			);
}

inline Matrix44f Ortho(float left, float right,
		float bottom, float top,
		float near, float far) {
	float tx = -(right + left) / (right - left);
	float ty = -(top + bottom) / (top - bottom);
	float tz = -(far + near) / (far - near);
	return Matrix44f(
			2.0f / (right - left), 0.0f, 0.0f, 0.0f,
			0, 2.0f / (top - bottom), 0.0f, 0.0f,
			0.0f, 0.0f, -2.0f / (far - near), 0.0f,
			tx, ty, tz, 1.0f
			);
}

inline Matrix44f Perspective(float fovy, float aspect,
		float near, float far) {
	float x = (fovy * M_PI / 180.0) / 2.0f;
	float f = cos(x) / sin(x);

	return Matrix44f(
			f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, f, 0.0f, 0.0f,
			0.0f, 0.0f, (far + near) / (near - far), -1.0f,
			0.0f, 0.0f, (2.0f * far * near) / (near - far), 0.0f
			);
}

inline Matrix44f Frustum(float left, float right,
		float bottom, float top,
		float near, float far) {
	float A = (right + left) / (right - left);
	float B = (top + bottom) / (top - bottom);
	float C = -(far + near) / (far - near);
	float D = - (2.0f * far * near) / (far - near);

	return Matrix44f(
			2.0f * near / (right - left), 0.0f, 0.0f, 0.0f,
			0.0f, 2.0f * near / (top - bottom), 0.0f, 0.0f,
			A, B, C, -1.0f,
			0.0f, 0.0f, D, 0.0f
			);
}

inline Matrix44f LookAt(
		const Vector3f& eye,
		const Vector3f& poi,
		const Vector3f& up) {
	Vector3f d = (poi - eye).normalized();
	Vector3f s = d.cross(up.normalized()).normalized();
	Vector3f u = s.cross(d).normalized();

	return TranslateMat44(-eye[0], -eye[1], -eye[2]) * Matrix44f(
			s[0], u[0], -d[0], 0.0f,
			s[1], u[1], -d[1], 0.0f,
			s[2], u[2], -d[2], 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
			);
}

/** Quaternion 
 *
 * order: x,y,z,w
 */
class Quaternion : public Vector4f {
	public:
		Quaternion () :
			Vector4f (0.f, 0.f, 0.f, 1.f)
		{}
		Quaternion (const Vector4f vec4) :
			Vector4f (vec4)
		{}
		Quaternion (float x, float y, float z, float w):
			Vector4f (x, y, z, w)
		{}
		/** This function is equivalent to multiplicate their corresponding rotation matrices */
		Quaternion operator* (const Quaternion &q) const {
			return Quaternion (
					(*this)[3] * q[0] + (*this)[0] * q[3] + (*this)[1] * q[2] - (*this)[2] * q[1],
					(*this)[3] * q[1] + (*this)[1] * q[3] + (*this)[2] * q[0] - (*this)[0] * q[2],
					(*this)[3] * q[2] + (*this)[2] * q[3] + (*this)[0] * q[1] - (*this)[1] * q[0],
					(*this)[3] * q[3] - (*this)[0] * q[0] - (*this)[1] * q[1] - (*this)[2] * q[2]
					);
		}
		Quaternion& operator*=(const Quaternion &q) {
			set (
					(*this)[3] * q[0] + (*this)[0] * q[3] + (*this)[1] * q[2] - (*this)[2] * q[1],
					(*this)[3] * q[1] + (*this)[1] * q[3] + (*this)[2] * q[0] - (*this)[0] * q[2],
					(*this)[3] * q[2] + (*this)[2] * q[3] + (*this)[0] * q[1] - (*this)[1] * q[0],
					(*this)[3] * q[3] - (*this)[0] * q[0] - (*this)[1] * q[1] - (*this)[2] * q[2]
					);
			return *this;
		}

		static Quaternion fromGLRotate (float angle, float x, float y, float z) {
			float st = sinf (angle * M_PI / 360.f);
			return Quaternion (
						st * x,
						st * y,
						st * z,
						cosf (angle * M_PI / 360.f)
						);
		}

		Quaternion normalize() {
			return Vector4f::normalize();
		}

		Quaternion slerp (float alpha, const Quaternion &quat) const {
			// check whether one of the two has 0 length
			float s = sqrt (squaredNorm() * quat.squaredNorm());

			// division by 0.f is unhealthy!
			assert (s != 0.f);

			float angle = acos (dot(quat) / s);
			if (angle == 0.f || std::isnan(angle)) {
				return *this;
			}
			assert(!std::isnan(angle));

			float d = 1.f / sinf (angle);
			float p0 = sinf ((1.f - alpha) * angle);
			float p1 = sinf (alpha * angle);

			if (dot (quat) < 0.f) {
				return Quaternion( ((*this) * p0 - quat * p1) * d);
			}
			return Quaternion( ((*this) * p0 + quat * p1) * d);
		}

		Matrix44f toGLMatrix() const {
			float x = (*this)[0];
			float y = (*this)[1];
			float z = (*this)[2];
			float w = (*this)[3];
			return Matrix44f (
					1 - 2*y*y - 2*z*z,
					2*x*y + 2*w*z,
					2*x*z - 2*w*y,
					0.f,

					2*x*y - 2*w*z,
					1 - 2*x*x - 2*z*z,
					2*y*z + 2*w*x,
					0.f,

					2*x*z + 2*w*y,
					2*y*z - 2*w*x,
					1 - 2*x*x - 2*y*y,
					0.f,
					
					0.f,
					0.f,
					0.f,
					1.f);
		}

		static Quaternion fromGLMatrix(const Matrix44f &mat) {
			float w = sqrt (1.f + mat(0,0) + mat(1,1) + mat(2,2)) * 0.5f;
			return Quaternion (
					-(mat(2,1) - mat(1,2)) / (w * 4.f),
					-(mat(0,2) - mat(2,0)) / (w * 4.f),
					-(mat(1,0) - mat(0,1)) / (w * 4.f),
					w);
		}

		static Quaternion fromMatrix (const Matrix33f &mat) {
			float w = sqrt (1.f + mat(0,0) + mat(1,1) + mat(2,2)) * 0.5f;
			return Quaternion (
					(mat(2,1) - mat(1,2)) / (w * 4.f),
					(mat(0,2) - mat(2,0)) / (w * 4.f),
					(mat(1,0) - mat(0,1)) / (w * 4.f),
					w);
		}

		static Quaternion fromAxisAngle (const Vector3f &axis, double angle_rad) {
			double d = axis.norm();
			double s2 = std::sin (angle_rad * 0.5) / d;
			return Quaternion (
					axis[0] * s2,
					axis[1] * s2,
					axis[2] * s2,
					std::cos(angle_rad * 0.5)
					);
		}

		static Quaternion fromEulerZYX (const Vector3f &zyx_angles) {
			return Quaternion::fromAxisAngle (Vector3f (0., 0., 1.), zyx_angles[0])
				* Quaternion::fromAxisAngle (Vector3f (0., 1., 0.), zyx_angles[1])
				* Quaternion::fromAxisAngle (Vector3f (1., 0., 0.), zyx_angles[2]); 
		}

		static Quaternion fromEulerYXZ (const Vector3f &yxz_angles) {
			return Quaternion::fromAxisAngle (Vector3f (0., 1., 0.), yxz_angles[0])
				* Quaternion::fromAxisAngle (Vector3f (1., 0., 0.), yxz_angles[1])
				* Quaternion::fromAxisAngle (Vector3f (0., 0., 1.), yxz_angles[2]);
		}

		static Quaternion fromEulerXYZ (const Vector3f &xyz_angles) {
			return Quaternion::fromAxisAngle (Vector3f (0., 0., 01.), xyz_angles[2]) 
				* Quaternion::fromAxisAngle (Vector3f (0., 1., 0.), xyz_angles[1])
				* Quaternion::fromAxisAngle (Vector3f (1., 0., 0.), xyz_angles[0]);
		}
 
		Vector3f toEulerZYX () const {
			return Vector3f (1.0f, 2.0f, 3.0f
					);
		}

		Vector3f toEulerYXZ() const {
			return Vector3f (
					atan2 (-2.f * (*this)[0] * (*this)[2] + 2.f * (*this)[3] * (*this)[1],
						(*this)[2] * (*this)[2] - (*this)[1] * (*this)[1]
						-(*this)[0] * (*this)[0] + (*this)[3] * (*this)[3]),
					asin (2.f * (*this)[1] * (*this)[2] + 2.f * (*this)[3] * (*this)[0]),
					atan2 (-2.f * (*this)[0] * (*this)[1] + 2.f * (*this)[3] * (*this)[2],
						(*this)[1] * (*this)[1] - (*this)[2] * (*this)[2]
						+(*this)[3] * (*this)[3] - (*this)[0] * (*this)[0]
						)
					);
		}

		Matrix33f toMatrix() const {
			float x = (*this)[0];
			float y = (*this)[1];
			float z = (*this)[2];
			float w = (*this)[3];
			return Matrix33f (
					1 - 2*y*y - 2*z*z,
					2*x*y - 2*w*z,
					2*x*z + 2*w*y,

					2*x*y + 2*w*z,
					1 - 2*x*x - 2*z*z,
					2*y*z - 2*w*x,

					2*x*z - 2*w*y,
					2*y*z + 2*w*x,
					1 - 2*x*x - 2*y*y
			);
		}

		Quaternion conjugate() const {
			return Quaternion (
					-(*this)[0],
					-(*this)[1],
					-(*this)[2],
					(*this)[3]);
		}

		Vector3f rotate (const Vector3f &vec) const {
			Vector3f vn (vec);
			Quaternion vec_quat (vn[0], vn[1], vn[2], 0.f), res_quat;

			res_quat = (*this) * vec_quat;
			res_quat = res_quat * conjugate();

			return Vector3f (res_quat[0], res_quat[1], res_quat[2]);
		}
};

// namespace GL
}

// namespace SimpleMath
}

/* _SIMPLEMATHGL_H_ */
#endif
