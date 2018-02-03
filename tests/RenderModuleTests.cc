#include <iostream>
#include "gtest/gtest.h"
#include "TestUtils.h"

#include "bx/math.h"
#include "src/math_types.h"
#include "src/modules/RenderModule.h"

using namespace std;

TEST(Transform, FromMatrix) {
	Matrix44f matrix;
	Vector3f angles (0.1f, -0.2f, 0.3f);
	Quaternion rotation = Quaternion::fromEulerXYZ(angles);
	Vector3f translation (1.0f, 2.0f, 3.0f);
	Vector3f scale (0.3f, 10.f, 20.319f);

	bx::mtxSRT(matrix.data(), 
			scale[0], scale[1], scale[2],
			angles[0], angles[1], angles[2],
			translation[0], translation[1], translation[2]
			);

	Transform transform;
	transform.fromMatrix(matrix);
	
	EXPECT_TRUE(MatrixClose(scale, transform.scale));
	EXPECT_TRUE(MatrixClose(translation, transform.translation));
	// We loose a lot of precision here and therefore accept a rather coarse
	// error.
	EXPECT_TRUE(MatrixClose(rotation, transform.rotation, 1.0e-1));
};

TEST(Transform, ToMatrix) {
	Matrix44f matrix;

	// transform data
	Vector3f angles (0.1f, -0.2f, 0.3f);
	Vector3f translation (1.0f, 2.0f, 3.0f);
	Vector3f scale (0.3f, 10.f, 20.319f);

	// compute matrix
	bx::mtxSRT(matrix.data(), 
			scale[0], scale[1], scale[2],
			angles[0], angles[1], angles[2],
			translation[0], translation[1], translation[2]
			);

	// extract values from transfrom
	Transform transform;
	transform.fromMatrix(matrix);

	// and transform back to matrix
	Matrix44f result = transform.toMatrix();

	EXPECT_TRUE(MatrixClose(matrix, result));
};
