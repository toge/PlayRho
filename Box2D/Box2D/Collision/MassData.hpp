/*
 * Original work Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
 * Modified work Copyright (c) 2016 Louis Langholtz https://github.com/louis-langholtz/Box2D
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef B2_MASS_DATA_HPP
#define B2_MASS_DATA_HPP

#include <Box2D/Common/Math.h>

namespace box2d {
	
	/// This holds the mass data computed for a shape.
	struct MassData
	{
		MassData() = default;
		
		/// Initializing constructor.
		/// @param m Non-negative mass in kg.
		/// @param c Position of the shape's centroid relative to the shape's origin.
		/// @param i Non-negative rotational inertia of the shape about the local origin.
		constexpr MassData(float_t m, Vec2 c, float_t i) noexcept: mass{m}, center{c}, I{i}
		{
			assert(mass >= 0);
			assert(I >= 0);
		}
		
		/// Mass of the shape in kilograms.
		/// This should NEVER be a negative value.
		/// @note Behavior is undefined if this value is negative.
		float_t mass;
		
		/// The position of the shape's centroid relative to the shape's origin.
		Vec2 center;
		
		/// The rotational inertia of the shape about the local origin.
		/// This should NEVER be a negative value.
		/// @note Behavior is undefined if this value is negative.
		float_t I;
	};
}

#endif /* B2_MASS_DATA_HPP */