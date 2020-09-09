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

#ifndef PLAYRHO_DYNAMICS_WORLD_HPP
#define PLAYRHO_DYNAMICS_WORLD_HPP

/// @file
/// Declarations of the World class and associated free functions.

#include <PlayRho/Common/Math.hpp>
#include <PlayRho/Common/Range.hpp> // for SizedRange
#include <PlayRho/Common/propagate_const.hpp>
#include <PlayRho/Collision/DynamicTree.hpp>
#include <PlayRho/Dynamics/BodyConf.hpp> // for GetDefaultBodyConf
#include <PlayRho/Dynamics/WorldCallbacks.hpp>
#include <PlayRho/Dynamics/StepStats.hpp>
#include <PlayRho/Dynamics/Contacts/ContactKey.hpp> // for KeyedContactPtr
#include <PlayRho/Dynamics/WorldConf.hpp>

#include <iterator>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <functional>

namespace playrho {

class StepConf;
struct IslandStats;

namespace d2 {

struct BodyConf;
struct JointConf;
class Body;
class Fixture;
class Joint;
class WorldImpl;

/// @defgroup PhysicalEntities Physical Entity Classes
///
/// @brief Classes representing physical entities typically created/destroyed via factory methods.
///
/// @details Classes of creatable and destroyable managed instances that associate
///   physical properties to simulations. These instances are typically created via a
///   method whose name begins with the prefix of <code>Create</code>. Similarly, these
///   instances are typically destroyed using a method whose name begins with the prefix
///   of <code>Destroy</code>.
///
/// @note For example, the following could be used to create a dynamic body having a one meter
///   radius disk shape:
/// @code{.cpp}
/// auto world = World{};
/// const auto body = world.CreateBody(BodyConf{}.UseType(BodyType::Dynamic));
/// const auto fixture = body->CreateFixture(Shape{DiskShapeConf{1_m}});
/// @endcode
///
/// @see World, World::CreateBody, World::CreateJoint, World::Destroy.
/// @see Body::CreateFixture, Body::Destroy, Body::DestroyFixtures.
/// @see BodyType, Shape, DiskShapeConf.

/// @brief Definition of an independent and simulatable "world".
///
/// @details The world class manages physics entities, dynamic simulation, and queries.
///   In a physical sense, perhaps this is more like a universe in that entities in a
///   world have no interaction with entities in other worlds. In any case, there's
///   precedence, from a physics-engine standpoint, for this being called a world.
///
/// @note World instances do not themselves have any force or acceleration properties.
///  They simply utilize the acceleration property of the bodies they manage. This is
///  different than some other engines (like <code>Box2D</code> which provides a world
///  gravity property).
/// @note World instances are composed of &mdash; i.e. contain and own &mdash; Body, Joint,
///   and Contact instances.
/// @note This data structure is 232-bytes large (with 4-byte Real on at least one 64-bit
///   platform).
/// @attention For example, the following could be used to create a dynamic body having a one meter
///   radius disk shape:
/// @code{.cpp}
/// auto world = World{};
/// const auto body = world.CreateBody(BodyConf{}.UseType(BodyType::Dynamic));
/// const auto fixture = body->CreateFixture(Shape{DiskShapeConf{1_m}});
/// @endcode
///
/// @see Body, Joint, Contact, PhysicalEntities.
///
class World
{
public:
    /// @brief Bodies container type.
    using Bodies = std::vector<Body*>;

    /// @brief Contacts container type.
    using Contacts = std::vector<KeyedContactPtr>;
    
    /// @brief Joints container type.
    /// @note Cannot be container of Joint instances since joints are polymorphic types.
    using Joints = std::vector<Joint*>;
    
    /// @brief Fixtures container type.
    using Fixtures = std::vector<Fixture*>;

    /// @brief Constructs a world object.
    /// @param def A customized world configuration or its default value.
    /// @note A lot more configurability can be had via the <code>StepConf</code>
    ///   data that's given to the world's <code>Step</code> method.
    /// @throws InvalidArgument if the given max vertex radius is less than the min.
    /// @see Step.
    explicit World(const WorldConf& def = GetDefaultWorldConf());

    /// @brief Copy constructor.
    /// @details Copy constructs this world with a deep copy of the given world.
    /// @post The state of this world is like that of the given world except this world now
    ///   has deep copies of the given world with pointers having the new addresses of the
    ///   new memory required for those copies.
    World(const World& other);

    World(World&& other) noexcept = delete;

    /// @brief Assignment operator.
    /// @details Copy assigns this world with a deep copy of the given world.
    /// @post The state of this world is like that of the given world except this world now
    ///   has deep copies of the given world with pointers having the new addresses of the
    ///   new memory required for those copies.
    /// @warning This method should not be called while the world is locked!
    /// @throws WrongState if this method is called while the world is locked.
    World& operator= (const World& other);

    World& operator= (World&& other) noexcept = delete;

    /// @brief Destructor.
    /// @details All physics entities are destroyed and all dynamically allocated memory
    ///    is released.
    ~World() noexcept;

    /// @brief Clears this world.
    /// @post The contents of this world have all been destroyed and this world's internal
    ///   state reset as though it had just been constructed.
    /// @throws WrongState if this method is called while the world is locked.
    void Clear();

    /// @brief Register a destruction listener.
    /// @note The listener is owned by you and must remain in scope.
    void SetDestructionListener(DestructionListener* listener) noexcept;

    /// @brief Register a contact event listener.
    /// @note The listener is owned by you and must remain in scope.
    void SetContactListener(ContactListener* listener) noexcept;

    /// @brief Creates a rigid body with the given configuration.
    /// @warning This function should not be used while the world is locked &mdash; as it is
    ///   during callbacks. If it is, it will throw an exception or abort your program.
    /// @note No references to the configuration are retained. Its value is copied.
    /// @post The created body will be present in the range returned from the
    ///   <code>GetBodies()</code> method.
    /// @param def A customized body configuration or its default value.
    /// @return Pointer to newly created body which can later be destroyed by calling the
    ///   <code>Destroy(Body*)</code> method.
    /// @throws WrongState if this method is called while the world is locked.
    /// @throws LengthError if this operation would create more than <code>MaxBodies</code>.
    /// @see Destroy(Body*), GetBodies.
    /// @see PhysicalEntities.
    Body* CreateBody(const BodyConf& def = GetDefaultBodyConf());

    /// @brief Creates a joint to constrain one or more bodies.
    /// @warning This function is locked during callbacks.
    /// @note No references to the configuration are retained. Its value is copied.
    /// @post The created joint will be present in the range returned from the
    ///   <code>GetJoints()</code> method.
    /// @return Pointer to newly created joint which can later be destroyed by calling the
    ///   <code>Destroy(Joint*)</code> method.
    /// @throws WrongState if this method is called while the world is locked.
    /// @throws LengthError if this operation would create more than <code>MaxJoints</code>.
    /// @throws InvalidArgument if the given definition is not allowed.
    /// @see PhysicalEntities.
    /// @see Destroy(Joint*), GetJoints.
    Joint* CreateJoint(const JointConf& def);

    /// @brief Destroys the given body.
    /// @details Destroys a given body that had previously been created by a call to this
    ///   world's <code>CreateBody(const BodyConf&)</code> method.
    /// @warning This automatically deletes all associated shapes and joints.
    /// @warning This function is locked during callbacks.
    /// @warning Behavior is undefined if given a null body.
    /// @warning Behavior is undefined if the passed body was not created by this world.
    /// @note This function is locked during callbacks.
    /// @post The destroyed body will no longer be present in the range returned from the
    ///   <code>GetBodies()</code> method.
    /// @post None of the body's fixtures will be present in the fixtures-for-proxies
    ///   collection.
    /// @param body Body to destroy that had been created by this world.
    /// @throws WrongState if this method is called while the world is locked.
    /// @see CreateBody(const BodyConf&), GetBodies, GetFixturesForProxies.
    /// @see PhysicalEntities.
    void Destroy(Body* body);

    /// @brief Destroys a joint.
    /// @details Destroys a given joint that had previously been created by a call to this
    ///   world's <code>CreateJoint(const JointConf&)</code> method.
    /// @warning This function is locked during callbacks.
    /// @warning Behavior is undefined if the passed joint was not created by this world.
    /// @note This may cause the connected bodies to begin colliding.
    /// @post The destroyed joint will no longer be present in the range returned from the
    ///   <code>GetJoints()</code> method.
    /// @param joint Joint to destroy that had been created by this world.
    /// @throws WrongState if this method is called while the world is locked.
    /// @see CreateJoint(const JointConf&), GetJoints.
    /// @see PhysicalEntities.
    void Destroy(Joint* joint);

    /// @brief Steps the world simulation according to the given configuration.
    ///
    /// @details
    /// Performs position and velocity updating, sleeping of non-moving bodies, updating
    /// of the contacts, and notifying the contact listener of begin-contact, end-contact,
    /// pre-solve, and post-solve events.
    ///
    /// @warning Behavior is undefined if given a negative step time delta.
    /// @warning Varying the step time delta may lead to non-physical behaviors.
    ///
    /// @note Calling this with a zero step time delta results only in fixtures and bodies
    ///   registered for proxy handling being processed. No physics is performed.
    /// @note If the given velocity and position iterations are zero, this method doesn't
    ///   do velocity or position resolutions respectively of the contacting bodies.
    /// @note While body velocities are updated accordingly (per the sum of forces acting on them),
    ///   body positions (barring any collisions) are updated as if they had moved the entire time
    ///   step at those resulting velocities. In other words, a body initially at position 0
    ///   (<code>p0</code>) going velocity 0 (<code>v0</code>) fast with a sum acceleration of
    ///   <code>a</code>, after time <code>t</code> and barring any collisions, will have a new
    ///   velocity (<code>v1</code>) of <code>v0 + (a * t)</code> and a new position
    ///   (<code>p1</code>) of <code>p0 + v1 * t</code>.
    ///
    /// @post Static bodies are unmoved.
    /// @post Kinetic bodies are moved based on their previous velocities.
    /// @post Dynamic bodies are moved based on their previous velocities, gravity, applied
    ///   forces, applied impulses, masses, damping, and the restitution and friction values
    ///   of their fixtures when they experience collisions.
    /// @post The bodies for proxies queue will be empty.
    /// @post The fixtures for proxies queue will be empty.
    ///
    /// @param conf Configuration for the simulation step.
    ///
    /// @return Statistics for the step.
    ///
    /// @throws WrongState if this method is called while the world is locked.
    ///
    /// @see GetBodiesForProxies, GetFixturesForProxies.
    ///
    StepStats Step(const StepConf& conf);

    /// @brief Gets the world body range for this world.
    /// @details Gets a range enumerating the bodies currently existing within this world.
    ///   These are the bodies that had been created from previous calls to the
    ///   <code>CreateBody(const BodyConf&)</code> method that haven't yet been destroyed.
    /// @return Body range that can be iterated over using its begin and end methods
    ///   or using ranged-based for-loops.
    /// @see CreateBody(const BodyConf&).
    SizedRange<Bodies::iterator> GetBodies() noexcept;

    /// @brief Gets the world body range for this constant world.
    /// @details Gets a range enumerating the bodies currently existing within this world.
    ///   These are the bodies that had been created from previous calls to the
    ///   <code>CreateBody(const BodyConf&)</code> method that haven't yet been destroyed.
    /// @return Body range that can be iterated over using its begin and end methods
    ///   or using ranged-based for-loops.
    /// @see CreateBody(const BodyConf&).
    SizedRange<Bodies::const_iterator> GetBodies() const noexcept;

    /// @brief Gets the bodies-for-proxies range for this world.
    /// @details Provides insight on what bodies have been queued for proxy processing
    ///   during the next call to the world step method.
    /// @see Step.
    SizedRange<Bodies::const_iterator> GetBodiesForProxies() const noexcept;

    /// @brief Gets the fixtures-for-proxies range for this world.
    /// @details Provides insight on what fixtures have been queued for proxy processing
    ///   during the next call to the world step method.
    /// @see Step.
    SizedRange<Fixtures::const_iterator> GetFixturesForProxies() const noexcept;

    /// @brief Gets the world joint range.
    /// @details Gets a range enumerating the joints currently existing within this world.
    ///   These are the joints that had been created from previous calls to the
    ///   <code>CreateJoint(const JointConf&)</code> method that haven't yet been destroyed.
    /// @return World joints sized-range.
    /// @see CreateJoint(const JointConf&).
    SizedRange<Joints::const_iterator> GetJoints() const noexcept;

    /// @brief Gets the world joint range.
    /// @details Gets a range enumerating the joints currently existing within this world.
    ///   These are the joints that had been created from previous calls to the
    ///   <code>CreateJoint(const JointConf&)</code> method that haven't yet been destroyed.
    /// @return World joints sized-range.
    /// @see CreateJoint(const JointConf&).
    SizedRange<Joints::iterator> GetJoints() noexcept;

    /// @brief Gets the world contact range.
    /// @warning contacts are created and destroyed in the middle of a time step.
    /// Use <code>ContactListener</code> to avoid missing contacts.
    /// @return World contacts sized-range.
    SizedRange<Contacts::const_iterator> GetContacts() const noexcept;
    
    /// @brief Whether or not "step" is complete.
    /// @details The "step" is completed when there are no more TOI events for the current time step.
    /// @return <code>true</code> unless sub-stepping is enabled and the step method returned
    ///   without finishing all of its sub-steps.
    /// @see GetSubStepping, SetSubStepping.
    bool IsStepComplete() const noexcept;
    
    /// @brief Gets whether or not sub-stepping is enabled.
    /// @see SetSubStepping, IsStepComplete.
    bool GetSubStepping() const noexcept;

    /// @brief Enables/disables single stepped continuous physics.
    /// @note This is not normally used. Enabling sub-stepping is meant for testing.
    /// @post The <code>GetSubStepping()</code> method will return the value this method was
    ///   called with.
    /// @see IsStepComplete, GetSubStepping.
    void SetSubStepping(bool flag) noexcept;

    /// @brief Gets access to the broad-phase dynamic tree information.
    const DynamicTree& GetTree() const noexcept;

    /// @brief Is the world locked (in the middle of a time step).
    bool IsLocked() const noexcept;

    /// @brief Shifts the world origin.
    /// @note Useful for large worlds.
    /// @note The body shift formula is: <code>position -= newOrigin</code>.
    /// @post The "origin" of this world's bodies, joints, and the board-phase dynamic tree
    ///   have been translated per the shift amount and direction.
    /// @param newOrigin the new origin with respect to the old origin
    /// @throws WrongState if this method is called while the world is locked.
    void ShiftOrigin(Length2 newOrigin);

    /// @brief Gets the minimum vertex radius that shapes in this world can be.
    Length GetMinVertexRadius() const noexcept;
    
    /// @brief Gets the maximum vertex radius that shapes in this world can be.
    Length GetMaxVertexRadius() const noexcept;

    /// @brief Gets the inverse delta time.
    /// @details Gets the inverse delta time that was set on construction or assignment, and
    ///   updated on every call to the <code>Step()</code> method having a non-zero delta-time.
    /// @see Step.
    Frequency GetInvDeltaTime() const noexcept;

private:
    friend class WorldAtty;

    propagate_const<std::unique_ptr<WorldImpl>> m_impl;
};

/// @example HelloWorld.cpp
/// This is the source file for the <code>HelloWorld</code> application that demonstrates
/// use of the playrho::d2::World class and more.

/// @example World.cpp
/// This is the <code>googletest</code> based unit testing file for the
/// <code>playrho::d2::World</code> class.

// Free functions.

/// @brief Gets the body count in the given world.
/// @return 0 or higher.
/// @relatedalso World
inline BodyCounter GetBodyCount(const World& world) noexcept
{
    return static_cast<BodyCounter>(size(world.GetBodies()));
}

/// Gets the count of joints in the given world.
/// @return 0 or higher.
/// @relatedalso World
inline JointCounter GetJointCount(const World& world) noexcept
{
    return static_cast<JointCounter>(size(world.GetJoints()));
}

/// @brief Gets the count of contacts in the given world.
/// @note Not all contacts are for shapes that are actually touching. Some contacts are for
///   shapes which merely have overlapping AABBs.
/// @return 0 or higher.
/// @relatedalso World
inline ContactCounter GetContactCount(const World& world) noexcept
{
    return static_cast<ContactCounter>(size(world.GetContacts()));
}

/// @brief Gets the touching count for the given world.
/// @relatedalso World
ContactCounter GetTouchingCount(const World& world) noexcept;

/// @brief Steps the world ahead by a given time amount.
///
/// @details Performs position and velocity updating, sleeping of non-moving bodies, updating
///   of the contacts, and notifying the contact listener of begin-contact, end-contact,
///   pre-solve, and post-solve events.
///   If the given velocity and position iterations are more than zero, this method also
///   respectively performs velocity and position resolution of the contacting bodies.
///
/// @note While body velocities are updated accordingly (per the sum of forces acting on them),
///   body positions (barring any collisions) are updated as if they had moved the entire time
///   step at those resulting velocities. In other words, a body initially at <code>p0</code>
///   going <code>v0</code> fast with a sum acceleration of <code>a</code>, after time
///   <code>t</code> and barring any collisions, will have a new velocity (<code>v1</code>) of
///   <code>v0 + (a * t)</code> and a new position (<code>p1</code>) of <code>p0 + v1 * t</code>.
///
/// @warning Varying the time step may lead to non-physical behaviors.
///
/// @post Static bodies are unmoved.
/// @post Kinetic bodies are moved based on their previous velocities.
/// @post Dynamic bodies are moved based on their previous velocities, gravity,
/// applied forces, applied impulses, masses, damping, and the restitution and friction values
/// of their fixtures when they experience collisions.
///
/// @param world World to step.
/// @param delta Time to simulate as a delta from the current state. This should not vary.
/// @param velocityIterations Number of iterations for the velocity constraint solver.
/// @param positionIterations Number of iterations for the position constraint solver.
///   The position constraint solver resolves the positions of bodies that overlap.
///
/// @relatedalso World
///
StepStats Step(World& world, Time delta,
               TimestepIters velocityIterations = 8,
               TimestepIters positionIterations = 3);

/// @brief Gets the count of fixtures in the given world.
/// @relatedalso World
std::size_t GetFixtureCount(const World& world) noexcept;

/// @brief Gets the count of unique shapes in the given world.
/// @relatedalso World
std::size_t GetShapeCount(const World& world) noexcept;

/// @brief Gets the count of awake bodies in the given world.
/// @relatedalso World
BodyCounter GetAwakeCount(const World& world) noexcept;

/// @brief Awakens all of the bodies in the given world.
/// @details Calls all of the world's bodies' <code>SetAwake</code> method.
/// @return Sum total of calls to bodies' <code>SetAwake</code> method that returned true.
/// @see Body::SetAwake.
/// @relatedalso World
BodyCounter Awaken(World& world) noexcept;

/// @brief Sets the accelerations of all the world's bodies.
/// @param world World instance to set the acceleration of all contained bodies for.
/// @param fn Function or functor with a signature like:
///   <code>Acceleration (*fn)(const Body& body)</code>.
/// @relatedalso World
template <class F>
void SetAccelerations(World& world, F fn) noexcept
{
    const auto bodies = world.GetBodies();
    std::for_each(begin(bodies), end(bodies), [&](World::Bodies::value_type &b) {
        SetAcceleration(GetRef(b), fn(GetRef(b)));
    });
}

/// @brief Sets the accelerations of all the world's bodies to the given value.
/// @relatedalso World
void SetAccelerations(World& world, Acceleration acceleration) noexcept;

/// @brief Sets the accelerations of all the world's bodies to the given value.
/// @note This will leave the angular acceleration alone.
/// @relatedalso World
void SetAccelerations(World& world, LinearAcceleration2 acceleration) noexcept;

/// @brief Clears forces.
/// @details Manually clear the force buffer on all bodies.
/// @relatedalso World
inline void ClearForces(World& world) noexcept
{
    SetAccelerations(world, Acceleration{
        LinearAcceleration2{0_mps2, 0_mps2}, 0 * RadianPerSquareSecond
    });
}

/// @brief Finds body in given world that's closest to the given location.
/// @relatedalso World
Body* FindClosestBody(const World& world, Length2 location) noexcept;

} // namespace d2

/// @brief Updates the given regular step statistics.
RegStepStats& Update(RegStepStats& lhs, const IslandStats& rhs) noexcept;

} // namespace playrho

#endif // PLAYRHO_DYNAMICS_WORLD_HPP
