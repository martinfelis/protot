#ifndef MESHUP_CONFIG_H
#define MESHUP_CONFIG_H

#include "SimpleMath/SimpleMath.h"

typedef SimpleMath::Fixed<float, 2, 1> Vector2f;
typedef SimpleMath::Fixed<float, 2, 2> Matrix22f;

typedef SimpleMath::Fixed<float, 3, 1> Vector3f;
typedef SimpleMath::Fixed<float, 3, 3> Matrix33f;

typedef SimpleMath::Fixed<float, 4, 1> Vector4f;
typedef SimpleMath::Fixed<float, 4, 4> Matrix44f;

typedef SimpleMath::Quaternion Quaternion;

typedef SimpleMath::Dynamic<float> VectorNf;
typedef SimpleMath::Dynamic<float> MatrixNNf;

typedef SimpleMath::Fixed<double, 3, 1> Vector3d;
typedef SimpleMath::Fixed<double, 3, 3> Matrix33d;

typedef SimpleMath::Dynamic<double> VectorNd;
typedef SimpleMath::Dynamic<double> MatrixNNd;

#endif
