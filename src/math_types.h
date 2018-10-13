#ifndef MESHUP_CONFIG_H
#define MESHUP_CONFIG_H

#include "SimpleMath/SimpleMath.h"

typedef SimpleMath::Matrix<float, 2, 1> Vector2f;
typedef SimpleMath::Matrix<float, 2, 2> Matrix22f;

typedef SimpleMath::Matrix<float, 3, 1> Vector3f;
typedef SimpleMath::Matrix<float, 3, 3> Matrix33f;

typedef SimpleMath::Matrix<float, 4, 1> Vector4f;
typedef SimpleMath::Matrix<float, 4, 4> Matrix44f;

typedef SimpleMath::GL::Quaternion Quaternion;

typedef SimpleMath::Matrix<float> VectorNf;
typedef SimpleMath::Matrix<float> MatrixNNf;

typedef SimpleMath::Matrix<double, 3, 1> Vector3d;
typedef SimpleMath::Matrix<double, 3, 3> Matrix33d;

typedef SimpleMath::Matrix<double> VectorNd;
typedef SimpleMath::Matrix<double> MatrixNNd;

#endif
