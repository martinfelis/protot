#pragma once

#include "math_types.h"

namespace ImGui {

namespace Protot {

// Returns a normalized vector where the value at the modified index
// is kept and only the other values are being modified so that the
// resulting vector is normalized.
bool DragFloat4Normalized(const char* label, float v[4], float v_speed = 1.0f, float v_min = 0.0f, float v_max = 0.0f, const char* display_format = "%.3f", float power = 1.0f)
{
	float old_values[4];
	memcpy (old_values, v, sizeof(float) * 4);

	bool modified = ImGui::DragFloat4(label, v, v_speed, v_min, v_max, display_format, power);
	if (modified) {
		int mod_index = -1;
		Vector3f other_values;
		int other_index = 0;

		// determine the modified index and copy the unmodified values to
		// other_values
		for (int i = 0; i < 4; ++i) {
			if (old_values[i] != v[i]) {
				mod_index = i;
			} else {
				other_values[other_index] = v[i];
				other_index++;
			}
		}

		// normalize, but take zero length of other values into account and
		// also modification of vectors with a single 1.
		float other_length = other_values.norm();
		if (fabs(v[mod_index]) >= 1.0f - 1.0e-6f || other_length == 0.0f) {
			other_values.setZero();
			v[mod_index] = 1.0f * v[mod_index] < 0.0f ? -1.0f : 1.0f; 
		} else {
			// normalize other_values to have the remaining length
			other_values = other_values * (1.f / other_length) * (sqrt(1.0f - v[mod_index] * v[mod_index]));
		}

		// construct the new vector
		other_index = 0;
		for (int i = 0; i < 4; ++i) {
			if (i != mod_index) {
				v[i] = other_values[other_index];
				other_index++;
			}
		}
	}

	return modified;
}

} // Protot

} // ImGui
