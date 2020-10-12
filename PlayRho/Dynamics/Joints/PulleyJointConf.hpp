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

#ifndef PLAYRHO_DYNAMICS_JOINTS_PULLEYJOINTCONF_HPP
#define PLAYRHO_DYNAMICS_JOINTS_PULLEYJOINTCONF_HPP

#include <PlayRho/Dynamics/Joints/JointConf.hpp>

#include <PlayRho/Common/Math.hpp>

namespace playrho {

struct ConstraintSolverConf;
class StepConf;

namespace d2 {

class Joint;
class World;
class BodyConstraint;

/// @brief Pulley joint definition.
/// @details This requires two ground anchors, two dynamic body anchor points, and a pulley ratio.
struct PulleyJointConf : public JointBuilder<PulleyJointConf>
{
    /// @brief Super type.
    using super = JointBuilder<PulleyJointConf>;

    PulleyJointConf() noexcept: super{super{}.UseCollideConnected(true)} {}

    /// Initialize the bodies, anchors, lengths, max lengths, and ratio using the world anchors.
    PulleyJointConf(BodyID bodyA, BodyID bodyB,
                    Length2 groundAnchorA = Length2{-1_m, +1_m}, Length2 groundAnchorB = Length2{+1_m, +1_m},
                    Length2 anchorA = Length2{-1_m, 0_m}, Length2 anchorB = Length2{+1_m, 0_m},
                    Length lA = 0_m, Length lB = 0_m);

    /// @brief Uses the given ratio value.
    constexpr auto& UseRatio(Real v) noexcept
    {
        ratio = v;
        return *this;
    }

    /// The first ground anchor in world coordinates. This point never moves.
    Length2 groundAnchorA = Length2{-1_m, +1_m};

    /// The second ground anchor in world coordinates. This point never moves.
    Length2 groundAnchorB = Length2{+1_m, +1_m};

    /// The local anchor point relative to body A's origin.
    Length2 localAnchorA = Length2{-1_m, 0_m};

    /// The local anchor point relative to body B's origin.
    Length2 localAnchorB = Length2{+1_m, 0_m};

    /// The a reference length for the segment attached to body-A.
    Length lengthA = 0_m;

    /// The a reference length for the segment attached to body-B.
    Length lengthB = 0_m;

    /// The pulley ratio, used to simulate a block-and-tackle.
    Real ratio = 1;

    Length constant = 0_m; ///< Constant.

    // Solver shared (between calls to InitVelocityConstraints).
    Momentum impulse = 0_Ns; ///< Impulse.

    // Solver temp (recalculated every call to InitVelocityConstraints).
    UnitVec uA; ///< Unit vector A.
    UnitVec uB; ///< Unit vector B.
    Length2 rA; ///< Relative A.
    Length2 rB; ///< Relative B.
    Mass mass; ///< Mass.
};

/// @brief Gets the definition data for the given joint.
/// @relatedalso Joint
PulleyJointConf GetPulleyJointConf(const Joint& joint);

/// @relatedalso World
PulleyJointConf GetPulleyJointConf(const World& world,
                                   BodyID bA, BodyID bB,
                                   Length2 groundA, Length2 groundB,
                                   Length2 anchorA, Length2 anchorB);

/// @relatedalso PulleyJointConf
constexpr Momentum2 GetLinearReaction(const PulleyJointConf& object) noexcept
{
    return object.impulse * object.uB;
}

/// @relatedalso PulleyJointConf
constexpr AngularMomentum GetAngularReaction(const PulleyJointConf&) noexcept
{
    return AngularMomentum{0};
}

/// @relatedalso PulleyJointConf
bool ShiftOrigin(PulleyJointConf& object, Length2 newOrigin) noexcept;

/// @brief Initializes velocity constraint data based on the given solver data.
/// @note This MUST be called prior to calling <code>SolveVelocity</code>.
/// @see SolveVelocity.
/// @relatedalso PulleyJointConf
void InitVelocity(PulleyJointConf& object, std::vector<BodyConstraint>& bodies,
                  const StepConf& step,
                  const ConstraintSolverConf& conf);

/// @brief Solves velocity constraint.
/// @pre <code>InitVelocity</code> has been called.
/// @see InitVelocity.
/// @return <code>true</code> if velocity is "solved", <code>false</code> otherwise.
/// @relatedalso PulleyJointConf
bool SolveVelocity(PulleyJointConf& object, std::vector<BodyConstraint>& bodies,
                   const StepConf& step);

/// @brief Solves the position constraint.
/// @return <code>true</code> if the position errors are within tolerance.
/// @relatedalso PulleyJointConf
bool SolvePosition(const PulleyJointConf& object, std::vector<BodyConstraint>& bodies,
                   const ConstraintSolverConf& conf);

/// @relatedalso PulleyJointConf
constexpr auto GetLengthA(const PulleyJointConf& object) noexcept
{
    return object.lengthA;
}

/// @relatedalso PulleyJointConf
constexpr auto GetLengthB(const PulleyJointConf& object) noexcept
{
    return object.lengthB;
}

} // namespace d2

template <>
struct TypeInfo<d2::PulleyJointConf>
{
    static const char* name() noexcept {
        return "d2::PulleyJointConf";
    }
};

} // namespace playrho

#endif // PLAYRHO_DYNAMICS_JOINTS_PULLEYJOINTCONF_HPP
