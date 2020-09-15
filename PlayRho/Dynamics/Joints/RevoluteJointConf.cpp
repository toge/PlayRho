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

#include <PlayRho/Dynamics/Joints/RevoluteJointConf.hpp>
#include <PlayRho/Dynamics/Joints/RevoluteJoint.hpp>
#include <PlayRho/Dynamics/Body.hpp>
#include <PlayRho/Dynamics/World.hpp>

namespace playrho {
namespace d2 {

RevoluteJointConf::RevoluteJointConf(BodyID bA, BodyID bB,
                                     Length2 laA, Length2 laB,
                                     Angle ra) noexcept:
    super{super{JointType::Revolute}.UseBodyA(bA).UseBodyB(bB)},
    localAnchorA{laA}, localAnchorB{laB}, referenceAngle{ra}
{
    // Intentionally empty.
}

RevoluteJointConf GetRevoluteJointConf(const RevoluteJoint& joint) noexcept
{
    auto def = RevoluteJointConf{};
    
    Set(def, joint);
    
    def.localAnchorA = joint.GetLocalAnchorA();
    def.localAnchorB = joint.GetLocalAnchorB();
    def.referenceAngle = joint.GetReferenceAngle();
    def.enableLimit = joint.IsLimitEnabled();
    def.lowerAngle = joint.GetLowerLimit();
    def.upperAngle = joint.GetUpperLimit();
    def.enableMotor = joint.IsMotorEnabled();
    def.motorSpeed = joint.GetMotorSpeed();
    def.maxMotorTorque = joint.GetMaxMotorTorque();
    
    return def;
}

RevoluteJointConf GetRevoluteJointConf(const World& world, BodyID bodyA, BodyID bodyB, Length2 anchor)
{
    return RevoluteJointConf{
        bodyA, bodyB,
        GetLocalPoint(world, bodyA, anchor),
        GetLocalPoint(world, bodyB, anchor),
        GetAngle(world, bodyB) - GetAngle(world, bodyA)
    };
}

} // namespace d2
} // namespace playrho
