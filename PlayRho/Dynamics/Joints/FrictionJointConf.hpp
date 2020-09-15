/*
 * Original work Copyright (c) 2006-2007 Erin Catto http://www.box2d.org
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

#ifndef PLAYRHO_DYNAMICS_JOINTS_FRICTIONJOINTCONF_HPP
#define PLAYRHO_DYNAMICS_JOINTS_FRICTIONJOINTCONF_HPP

#include <PlayRho/Dynamics/Joints/JointConf.hpp>

#include <PlayRho/Common/NonNegative.hpp>
#include <PlayRho/Common/Math.hpp>

namespace playrho {
namespace d2 {

class World;
class FrictionJoint;

/// @brief Friction joint definition.
struct FrictionJointConf : public JointBuilder<FrictionJointConf>
{
    /// @brief Super type.
    using super = JointBuilder<FrictionJointConf>;

    constexpr FrictionJointConf() noexcept: super{JointType::Friction} {}

    /// @brief Initializing constructor.
    /// @details Initialize the bodies, anchors, axis, and reference angle using the world
    ///   anchor and world axis.
    FrictionJointConf(BodyID bodyA, BodyID bodyB,
                      Length2 laA = Length2{}, Length2 laB = Length2{}) noexcept;

    /// @brief Uses the given maximum force value.
    constexpr FrictionJointConf& UseMaxForce(NonNegative<Force> v) noexcept;

    /// @brief Uses the given maximum torque value.
    constexpr FrictionJointConf& UseMaxTorque(NonNegative<Torque> v) noexcept;

    /// @brief Local anchor point relative to body A's origin.
    Length2 localAnchorA = Length2{};

    /// @brief Local anchor point relative to body B's origin.
    Length2 localAnchorB = Length2{};

    /// @brief Maximum friction force.
    NonNegative<Force> maxForce = NonNegative<Force>{0_N};

    /// @brief Maximum friction torque.
    NonNegative<Torque> maxTorque = NonNegative<Torque>{0_Nm};
};

constexpr FrictionJointConf& FrictionJointConf::UseMaxForce(NonNegative<Force> v) noexcept
{
    maxForce = v;
    return *this;
}

constexpr FrictionJointConf& FrictionJointConf::UseMaxTorque(NonNegative<Torque> v) noexcept
{
    maxTorque = v;
    return *this;
}

/// @brief Gets the definition data for the given joint.
/// @relatedalso FrictionJoint
FrictionJointConf GetFrictionJointConf(const FrictionJoint& joint) noexcept;

FrictionJointConf GetFrictionJointConf(const World& world,
                                       BodyID bodyA, BodyID bodyB, Length2 anchor);

} // namespace d2
} // namespace playrho

#endif // PLAYRHO_DYNAMICS_JOINTS_FRICTIONJOINTCONF_HPP
