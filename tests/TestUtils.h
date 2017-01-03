#include <limits>

template<typename T>
::testing::AssertionResult MatrixClose(
		const T &expected,
		const T &actual,
		float relative_error_precision = 1.0e-6){
	if (expected.rows() != actual.rows()) {
		return ::testing::AssertionFailure() << "matrices have different sizes: expected " << expected.rows() << " rows but have " << actual.rows();
	}

	if (expected.cols() != actual.cols()) {
		return ::testing::AssertionFailure() << "matrices have different sizes: expected " << expected.cols() << " columns but have " << actual.cols();
	}

	// compute relative error component wise
	size_t fail_i = 0;
	size_t fail_j = 0;
	bool failed = false;
	for (size_t i = 0; i < expected.rows(); ++i) {
		for (size_t j = 0; j < expected.cols(); ++j) {
			float diff = fabs(expected(i,j) - actual(i,j));

			// special case: expected is 0. In that we cannot normalize
			if (
					(expected(i,j) == 0.0f && diff > relative_error_precision)
					|| (diff / fabs(expected(i,j)) > relative_error_precision)
					) {
				failed = true;
				fail_j = j;
				break;
			}
		}

		if (failed) {
			fail_i = i;
			break;
		}
	}

	if (failed) {
		return ::testing::AssertionFailure() << "matrix " << std::endl
			<< actual << std::endl << " is not close to expected matrix " << std::endl
			<< expected << std::endl;
	}

	return ::testing::AssertionSuccess();
}

template<typename T, size_t size>
::testing::AssertionResult ArraysMatch(const T (&expected)[size],
		const T (&actual)[size]){
	for (size_t i(0); i < size; ++i){
		if (expected[i] != actual[i]){
			return ::testing::AssertionFailure() << "array[" << i
				<< "] (" << actual[i] << ") != expected[" << i
				<< "] (" << expected[i] << ")";
		}
	}

	return ::testing::AssertionSuccess();
}
