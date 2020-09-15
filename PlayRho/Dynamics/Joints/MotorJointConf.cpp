/*
 * Original work Copyright (c) 2006-2012 Erin Catto http://www.box2d.org
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

#include <PlayRho/Dynamics/Joints/MotorJointConf.hpp>

#include <PlayRho/Dynamics/Joints/MotorJoint.hpp>
#include <PlayRho/Dynamics/World.hpp>

namespace playrho {
namespace d2 {

MotorJointConf::MotorJointConf(BodyID bA, BodyID bB, Length2 lo, Angle ao) noexcept:
    super{super{JointType::Motor}.UseBodyA(bA).UseBodyB(bB)},
    linearOffset{lo}, angularOffset{ao}
{
    // Intentionally empty.
}

MotorJointConf GetMotorJointConf(const MotorJoint& joint) noexcept
{
    auto def = MotorJointConf{};
    Set(def, joint);
    def.linearOffset = joint.GetLinearOffset();
    def.angularOffset = joint.GetAngularOffset();
    def.maxForce = joint.GetMaxForce();
    def.maxTorque = joint.GetMaxTorque();
    def.correctionFactor = joint.GetCorrectionFactor();
    return def;
}

MotorJointConf GetMotorJointConf(const World& world, BodyID bA, BodyID bB)
{
    return MotorJointConf{
        bA, bB,
        GetLocalPoint(world, bA, GetLocation(world, bB)),
        GetAngle(world, bB) - GetAngle(world, bA)
    };
}

} // namespace d2
} // namespace playrho
