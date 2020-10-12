/*
 * Original work Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
 * Modified work Copyright (c) 2017 Louis Langholtz https://github.com/louis-langholtz/PlayRho
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef PLAYRHO_DYNAMICS_JOINTS_GEARJOINTCONF_HPP
#define PLAYRHO_DYNAMICS_JOINTS_GEARJOINTCONF_HPP

#include <PlayRho/Dynamics/Joints/JointConf.hpp>

#include <PlayRho/Common/Math.hpp>
#include <PlayRho/Dynamics/Joints/JointID.hpp>

namespace playrho {

struct ConstraintSolverConf;
class StepConf;

namespace d2 {

class Joint;
class World;
class BodyConstraint;

/// @brief Gear joint definition.
/// @details This definition requires two existing
/// revolute or prismatic joints (any combination will work).
struct GearJointConf : public JointBuilder<GearJointConf>
{
    /// @brief Super type.
    using super = JointBuilder<GearJointConf>;

    /// @brief Initializing constructor.
    GearJointConf(BodyID bA, BodyID bB, BodyID bC, BodyID bD) noexcept;

    /// @brief Uses the given ratio value.
    constexpr auto& UseRatio(Real v) noexcept
    {
        ratio = v;
        return *this;
    }

    BodyID bodyC = InvalidBodyID;
    BodyID bodyD = InvalidBodyID;

    JointType type1 = GetTypeID<void>();
    JointType type2 = GetTypeID<void>();

    // Used when not Revolute...
    Length2 localAnchorA{}; ///< Local anchor A.
    Length2 localAnchorB{}; ///< Local anchor B.
    Length2 localAnchorC{}; ///< Local anchor C.
    Length2 localAnchorD{}; ///< Local anchor D.
    
    UnitVec localAxis1; ///< Local axis 1. Used when type1 is not Revolute.
    UnitVec localAxis2; ///< Local axis 2. Used when type2 is not Revolute.

    Angle referenceAngle1; ///< Reference angle of joint 1. Used when type1 is Revolute.
    Angle referenceAngle2; ///< Reference angle of joint 2. Used when type2 is Revolute.

    /// The gear ratio.
    /// @see GearJoint for explanation.
    Real ratio = Real{1};

    Real constant = Real{0};

    Momentum impulse = 0_Ns; ///< Impulse.

    // Solver temp
    Vec2 JvAC = Vec2{}; ///< <code>AC Jv</code> data.
    Vec2 JvBD = {}; ///< <code>BD Jv</code> data.
    Length JwA = 0_m; ///< A <code>Jw</code> data.
    Length JwB = 0_m; ///< B <code>Jw</code> data.
    Length JwC = 0_m; ///< C <code>Jw</code> data.
    Length JwD = 0_m; ///< D <code>Jw</code> data.
    Real mass = 0; ///< Either linear mass or angular mass.
};

/// @brief Gets the definition data for the given joint.
/// @relatedalso Joint
GearJointConf GetGearJointConf(const Joint& joint) noexcept;

/// @relatedalso World
GearJointConf GetGearJointConf(const World& world, JointID id1, JointID id2, Real ratio = Real{1});

/// @relatedalso GearJointConf
constexpr Momentum2 GetLinearReaction(const GearJointConf& object)
{
    return object.impulse * object.JvAC;
}

/// @relatedalso GearJointConf
constexpr AngularMomentum GetAngularReaction(const GearJointConf& object)
{
    return object.impulse * object.JwA / Radian;
}

/// @relatedalso GearJointConf
constexpr bool ShiftOrigin(GearJointConf&, Length2) noexcept
{
    return false;
}

/// @brief Initializes velocity constraint data based on the given solver data.
/// @note This MUST be called prior to calling <code>SolveVelocity</code>.
/// @see SolveVelocity.
/// @relatedalso GearJointConf
void InitVelocity(GearJointConf& object, std::vector<BodyConstraint>& bodies,
                  const StepConf& step,
                  const ConstraintSolverConf& conf);

/// @brief Solves velocity constraint.
/// @pre <code>InitVelocity</code> has been called.
/// @see InitVelocity.
/// @return <code>true</code> if velocity is "solved", <code>false</code> otherwise.
/// @relatedalso GearJointConf
bool SolveVelocity(GearJointConf& object, std::vector<BodyConstraint>& bodies,
                   const StepConf& step);

/// @brief Solves the position constraint.
/// @return <code>true</code> if the position errors are within tolerance.
/// @relatedalso GearJointConf
bool SolvePosition(const GearJointConf& object, std::vector<BodyConstraint>& bodies,
                   const ConstraintSolverConf& conf);

/// @relatedalso GearJointConf
constexpr auto GetRatio(const GearJointConf& object) noexcept
{
    return object.ratio;
}

/// @relatedalso GearJointConf
constexpr auto SetRatio(GearJointConf& object, Real value) noexcept
{
    object.UseRatio(value);
}

/// @relatedalso GearJointConf
constexpr auto GetConstant(const GearJointConf& object) noexcept
{
    return object.constant;
}

/// @relatedalso GearJointConf
constexpr auto GetType1(const GearJointConf& object) noexcept
{
    return object.type1;
}

/// @relatedalso GearJointConf
constexpr auto GetType2(const GearJointConf& object) noexcept
{
    return object.type2;
}

} // namespace d2

template <>
struct TypeInfo<d2::GearJointConf>
{
    static const char* name() noexcept {
        return "d2::GearJointConf";
    }
};

} // namespace playrho

#endif // PLAYRHO_DYNAMICS_JOINTS_GEARJOINTCONF_HPP
