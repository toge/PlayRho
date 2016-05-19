/*
* Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
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

#include <Box2D/Dynamics/Joints/b2WeldJoint.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2TimeStep.h>

using namespace box2d;

// Point-to-point constraint
// C = p2 - p1
// Cdot = v2 - v1
//      = v2 + cross(w2, r2) - v1 - cross(w1, r1)
// J = [-I -r1_skew I r2_skew ]
// Identity used:
// w k % (rx i + ry j) = w * (-ry i + rx j)

// Angle constraint
// C = angle2 - angle1 - referenceAngle
// Cdot = w2 - w1
// J = [0 0 -1 0 0 1]
// K = invI1 + invI2

void WeldJointDef::Initialize(Body* bA, Body* bB, const Vec2& anchor)
{
	bodyA = bA;
	bodyB = bB;
	localAnchorA = bodyA->GetLocalPoint(anchor);
	localAnchorB = bodyB->GetLocalPoint(anchor);
	referenceAngle = bodyB->GetAngle() - bodyA->GetAngle();
}

WeldJoint::WeldJoint(const WeldJointDef* def)
: Joint(def)
{
	m_localAnchorA = def->localAnchorA;
	m_localAnchorB = def->localAnchorB;
	m_referenceAngle = def->referenceAngle;
	m_frequencyHz = def->frequencyHz;
	m_dampingRatio = def->dampingRatio;

	m_impulse = Vec3_zero;
}

void WeldJoint::InitVelocityConstraints(const b2SolverData& data)
{
	m_indexA = m_bodyA->m_islandIndex;
	m_indexB = m_bodyB->m_islandIndex;
	m_localCenterA = m_bodyA->m_sweep.localCenter;
	m_localCenterB = m_bodyB->m_sweep.localCenter;
	m_invMassA = m_bodyA->m_invMass;
	m_invMassB = m_bodyB->m_invMass;
	m_invIA = m_bodyA->m_invI;
	m_invIB = m_bodyB->m_invI;

	const auto aA = data.positions[m_indexA].a;
	auto vA = data.velocities[m_indexA].v;
	auto wA = data.velocities[m_indexA].w;

	const auto aB = data.positions[m_indexB].a;
	auto vB = data.velocities[m_indexB].v;
	auto wB = data.velocities[m_indexB].w;

	const Rot qA(aA), qB(aB);

	m_rA = Mul(qA, m_localAnchorA - m_localCenterA);
	m_rB = Mul(qB, m_localAnchorB - m_localCenterB);

	// J = [-I -r1_skew I r2_skew]
	//     [ 0       -1 0       1]
	// r_skew = [-ry; rx]

	// Matlab
	// K = [ mA+r1y^2*iA+mB+r2y^2*iB,  -r1y*iA*r1x-r2y*iB*r2x,          -r1y*iA-r2y*iB]
	//     [  -r1y*iA*r1x-r2y*iB*r2x, mA+r1x^2*iA+mB+r2x^2*iB,           r1x*iA+r2x*iB]
	//     [          -r1y*iA-r2y*iB,           r1x*iA+r2x*iB,                   iA+iB]

	const auto mA = m_invMassA, mB = m_invMassB;
	const auto iA = m_invIA, iB = m_invIB;

	Mat33 K;
	K.ex.x = mA + mB + m_rA.y * m_rA.y * iA + m_rB.y * m_rB.y * iB;
	K.ey.x = -m_rA.y * m_rA.x * iA - m_rB.y * m_rB.x * iB;
	K.ez.x = -m_rA.y * iA - m_rB.y * iB;
	K.ex.y = K.ey.x;
	K.ey.y = mA + mB + m_rA.x * m_rA.x * iA + m_rB.x * m_rB.x * iB;
	K.ez.y = m_rA.x * iA + m_rB.x * iB;
	K.ex.z = K.ez.x;
	K.ey.z = K.ez.y;
	K.ez.z = iA + iB;

	if (m_frequencyHz > float_t{0})
	{
		K.GetInverse22(&m_mass);

		auto invM = iA + iB;
		const auto m = (invM > float_t{0}) ? float_t(1) / invM : float_t{0};

		const auto C = aB - aA - m_referenceAngle;

		// Frequency
		const auto omega = float_t(2) * Pi * m_frequencyHz;

		// Damping coefficient
		const auto d = float_t(2) * m * m_dampingRatio * omega;

		// Spring stiffness
		const auto k = m * omega * omega;

		// magic formulas
		const auto h = data.step.get_dt();
		m_gamma = h * (d + h * k);
		m_gamma = (m_gamma != float_t{0}) ? float_t(1) / m_gamma : float_t{0};
		m_bias = C * h * k * m_gamma;

		invM += m_gamma;
		m_mass.ez.z = (invM != float_t{0}) ? float_t(1) / invM : float_t{0};
	}
	else if (K.ez.z == float_t{0})
	{
		K.GetInverse22(&m_mass);
		m_gamma = float_t{0};
		m_bias = float_t{0};
	}
	else
	{
		K.GetSymInverse33(&m_mass);
		m_gamma = float_t{0};
		m_bias = float_t{0};
	}

	if (data.step.warmStarting)
	{
		// Scale impulses to support a variable time step.
		m_impulse *= data.step.dtRatio;

		const auto P = Vec2(m_impulse.x, m_impulse.y);

		vA -= mA * P;
		wA -= iA * (Cross(m_rA, P) + m_impulse.z);

		vB += mB * P;
		wB += iB * (Cross(m_rB, P) + m_impulse.z);
	}
	else
	{
		m_impulse = Vec3_zero;
	}

	data.velocities[m_indexA].v = vA;
	data.velocities[m_indexA].w = wA;
	data.velocities[m_indexB].v = vB;
	data.velocities[m_indexB].w = wB;
}

void WeldJoint::SolveVelocityConstraints(const b2SolverData& data)
{
	auto vA = data.velocities[m_indexA].v;
	auto wA = data.velocities[m_indexA].w;
	auto vB = data.velocities[m_indexB].v;
	auto wB = data.velocities[m_indexB].w;

	const auto mA = m_invMassA, mB = m_invMassB;
	const auto iA = m_invIA, iB = m_invIB;

	if (m_frequencyHz > float_t{0})
	{
		const auto Cdot2 = wB - wA;

		const auto impulse2 = -m_mass.ez.z * (Cdot2 + m_bias + m_gamma * m_impulse.z);
		m_impulse.z += impulse2;

		wA -= iA * impulse2;
		wB += iB * impulse2;

		const auto Cdot1 = vB + Cross(wB, m_rB) - vA - Cross(wA, m_rA);

		const auto impulse1 = -Mul22(m_mass, Cdot1);
		m_impulse.x += impulse1.x;
		m_impulse.y += impulse1.y;

		const auto P = impulse1;

		vA -= mA * P;
		wA -= iA * Cross(m_rA, P);

		vB += mB * P;
		wB += iB * Cross(m_rB, P);
	}
	else
	{
		const auto Cdot1 = vB + Cross(wB, m_rB) - vA - Cross(wA, m_rA);
		const auto Cdot2 = wB - wA;
		const auto Cdot = Vec3(Cdot1.x, Cdot1.y, Cdot2);

		const auto impulse = -Mul(m_mass, Cdot);
		m_impulse += impulse;

		const auto P = Vec2(impulse.x, impulse.y);

		vA -= mA * P;
		wA -= iA * (Cross(m_rA, P) + impulse.z);

		vB += mB * P;
		wB += iB * (Cross(m_rB, P) + impulse.z);
	}

	data.velocities[m_indexA].v = vA;
	data.velocities[m_indexA].w = wA;
	data.velocities[m_indexB].v = vB;
	data.velocities[m_indexB].w = wB;
}

bool WeldJoint::SolvePositionConstraints(const b2SolverData& data)
{
	auto cA = data.positions[m_indexA].c;
	auto aA = data.positions[m_indexA].a;
	auto cB = data.positions[m_indexB].c;
	auto aB = data.positions[m_indexB].a;

	const Rot qA(aA), qB(aB);

	const auto mA = m_invMassA, mB = m_invMassB;
	const auto iA = m_invIA, iB = m_invIB;

	const auto rA = Mul(qA, m_localAnchorA - m_localCenterA);
	const auto rB = Mul(qB, m_localAnchorB - m_localCenterB);

	float_t positionError, angularError;

	Mat33 K;
	K.ex.x = mA + mB + rA.y * rA.y * iA + rB.y * rB.y * iB;
	K.ey.x = -rA.y * rA.x * iA - rB.y * rB.x * iB;
	K.ez.x = -rA.y * iA - rB.y * iB;
	K.ex.y = K.ey.x;
	K.ey.y = mA + mB + rA.x * rA.x * iA + rB.x * rB.x * iB;
	K.ez.y = rA.x * iA + rB.x * iB;
	K.ex.z = K.ez.x;
	K.ey.z = K.ez.y;
	K.ez.z = iA + iB;

	if (m_frequencyHz > float_t{0})
	{
		const auto C1 =  cB + rB - cA - rA;

		positionError = C1.Length();
		angularError = float_t{0};

		const auto P = -K.Solve22(C1);

		cA -= mA * P;
		aA -= iA * Cross(rA, P);

		cB += mB * P;
		aB += iB * Cross(rB, P);
	}
	else
	{
		const auto C1 =  cB + rB - cA - rA;
		const auto C2 = aB - aA - m_referenceAngle;

		positionError = C1.Length();
		angularError = Abs(C2);

		const auto C = Vec3(C1.x, C1.y, C2);
	
		Vec3 impulse;
		if (K.ez.z > float_t{0})
		{
			impulse = -K.Solve33(C);
		}
		else
		{
			const auto impulse2 = -K.Solve22(C1);
			impulse = Vec3(impulse2.x, impulse2.y, float_t{0});
		}

		const auto P = Vec2(impulse.x, impulse.y);

		cA -= mA * P;
		aA -= iA * (Cross(rA, P) + impulse.z);

		cB += mB * P;
		aB += iB * (Cross(rB, P) + impulse.z);
	}

	data.positions[m_indexA].c = cA;
	data.positions[m_indexA].a = aA;
	data.positions[m_indexB].c = cB;
	data.positions[m_indexB].a = aB;

	return (positionError <= LinearSlop) && (angularError <= AngularSlop);
}

Vec2 WeldJoint::GetAnchorA() const
{
	return m_bodyA->GetWorldPoint(m_localAnchorA);
}

Vec2 WeldJoint::GetAnchorB() const
{
	return m_bodyB->GetWorldPoint(m_localAnchorB);
}

Vec2 WeldJoint::GetReactionForce(float_t inv_dt) const
{
	const auto P = Vec2(m_impulse.x, m_impulse.y);
	return inv_dt * P;
}

float_t WeldJoint::GetReactionTorque(float_t inv_dt) const
{
	return inv_dt * m_impulse.z;
}

void WeldJoint::Dump()
{
	const auto indexA = m_bodyA->m_islandIndex;
	const auto indexB = m_bodyB->m_islandIndex;

	log("  WeldJointDef jd;\n");
	log("  jd.bodyA = bodies[%d];\n", indexA);
	log("  jd.bodyB = bodies[%d];\n", indexB);
	log("  jd.collideConnected = bool(%d);\n", m_collideConnected);
	log("  jd.localAnchorA = Vec2(%.15lef, %.15lef);\n", m_localAnchorA.x, m_localAnchorA.y);
	log("  jd.localAnchorB = Vec2(%.15lef, %.15lef);\n", m_localAnchorB.x, m_localAnchorB.y);
	log("  jd.referenceAngle = %.15lef;\n", m_referenceAngle);
	log("  jd.frequencyHz = %.15lef;\n", m_frequencyHz);
	log("  jd.dampingRatio = %.15lef;\n", m_dampingRatio);
	log("  joints[%d] = m_world->CreateJoint(&jd);\n", m_index);
}
