/*
 * Original work Copyright (c) 2006-2011 Erin Catto http://www.box2d.org
 * Modified work Copyright (c) 2021 Louis Langholtz https://github.com/louis-langholtz/PlayRho
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

#ifndef PLAYRHO_DYNAMICS_WORLDIMPL_HPP
#define PLAYRHO_DYNAMICS_WORLDIMPL_HPP

/// @file
/// Declarations of the WorldImpl class.

#include <PlayRho/Common/Math.hpp>
#include <PlayRho/Common/Positive.hpp>
#include <PlayRho/Common/ArrayAllocator.hpp>

#include <PlayRho/Collision/DynamicTree.hpp>
#include <PlayRho/Collision/MassData.hpp>

#include <PlayRho/Dynamics/BodyID.hpp>
#include <PlayRho/Dynamics/Filter.hpp>
#include <PlayRho/Dynamics/Island.hpp>
#include <PlayRho/Dynamics/StepStats.hpp>
#include <PlayRho/Dynamics/Contacts/BodyConstraint.hpp>
#include <PlayRho/Dynamics/Contacts/ContactKey.hpp>
#include <PlayRho/Dynamics/Contacts/KeyedContactID.hpp> // for KeyedContactPtr
#include <PlayRho/Dynamics/WorldConf.hpp>
#include <PlayRho/Dynamics/Joints/JointID.hpp>
#include <PlayRho/Dynamics/IslandStats.hpp>
#include <PlayRho/Collision/Shapes/ShapeID.hpp>

#include <iterator>
#include <vector>
#include <map>
#include <memory>
#include <stack>
#include <stdexcept>
#include <functional>

namespace playrho {

struct StepConf;
enum class BodyType;
class Contact;

namespace d2 {

class Body;
class Joint;
class Shape;
class Manifold;
class ContactImpulsesList;

/// @brief Definition of a "world" implementation.
/// @see World.
class WorldImpl {
public:
    /// @brief Bodies container type.
    using Bodies = std::vector<BodyID>;

    /// @brief Contacts container type.
    using Contacts = std::vector<KeyedContactPtr>;

    /// @brief Joints container type.
    /// @note Cannot be container of Joint instances since joints are polymorphic types.
    using Joints = std::vector<JointID>;

    /// @brief Body joints container type.
    using BodyJoints = std::vector<std::pair<BodyID, JointID>>;

    /// @brief Proxy ID type alias.
    using ProxyId = DynamicTree::Size;

    /// @brief Proxy container type alias.
    using Proxies = std::vector<ProxyId>;

    /// @brief Shape listener.
    using ShapeListener = std::function<void(ShapeID)>;

    /// @brief Body-shape listener.
    using AssociationListener = std::function<void(std::pair<BodyID, ShapeID>)>;

    /// @brief Joint listener.
    using JointListener = std::function<void(JointID)>;

    /// @brief Contact listener.
    using ContactListener = std::function<void(ContactID)>;

    /// @brief Manifold contact listener.
    using ManifoldContactListener = std::function<void(ContactID, const Manifold&)>;

    /// @brief Impulses contact listener.
    using ImpulsesContactListener = std::function<void(ContactID, const ContactImpulsesList&, unsigned)>;

    struct ContactUpdateConf;

    /// @name Special Member Functions
    /// Special member functions that are explicitly defined.
    /// @{

    /// @brief Constructs a world implementation for a world.
    /// @param def A customized world configuration or its default value.
    /// @note A lot more configurability can be had via the <code>StepConf</code>
    ///   data that's given to the world's <code>Step</code> method.
    /// @throws InvalidArgument if the given max vertex radius is less than the min.
    /// @see Step.
    explicit WorldImpl(const WorldConf& def = GetDefaultWorldConf());

    /// @brief Copy constructor.
    /// @details Copy constructs this world with a deep copy of the given world.
    WorldImpl(const WorldImpl& other) = default;

    /// @brief Assignment operator.
    /// @details Copy assigns this world with a deep copy of the given world.
    WorldImpl& operator=(const WorldImpl& other) = default;

    /// @brief Destructor.
    /// @details All physics entities are destroyed and all memory is released.
    /// @note This will call the <code>Clear()</code> function.
    /// @see Clear.
    ~WorldImpl() noexcept;

    /// @}

    /// @name Listener Member Functions
    /// @{

    /// @brief Registers a destruction listener for shapes.
    void SetShapeDestructionListener(ShapeListener listener) noexcept;

    /// @brief Registers a detach listener for shapes detaching from bodies.
    void SetDetachListener(AssociationListener listener) noexcept;

    /// @brief Register a destruction listener for joints.
    void SetJointDestructionListener(JointListener listener) noexcept;

    /// @brief Register a begin contact event listener.
    void SetBeginContactListener(ContactListener listener) noexcept;

    /// @brief Register an end contact event listener.
    void SetEndContactListener(ContactListener listener) noexcept;

    /// @brief Register a pre-solve contact event listener.
    void SetPreSolveContactListener(ManifoldContactListener listener) noexcept;

    /// @brief Register a post-solve contact event listener.
    void SetPostSolveContactListener(ImpulsesContactListener listener) noexcept;

    /// @}

    /// @name Miscellaneous Member Functions
    /// @{

    /// @brief Clears this world.
    /// @post The contents of this world have all been destroyed and this world's internal
    ///   state reset as though it had just been constructed.
    void Clear() noexcept;

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
    /// @throws WrongState if this function is called while the world is locked.
    ///
    /// @see GetBodiesForProxies, GetFixturesForProxies.
    ///
    StepStats Step(const StepConf& conf);

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
    /// @throws WrongState if this function is called while the world is locked.
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

    /// @brief Gets the dynamic tree leaves queued for finding new contacts.
    /// @see FindNewContacts, AddProxies.
    const Proxies& GetProxies() const noexcept;

    /// @brief Adds the given dynamic tree leaves to the queue for finding new contacts.
    /// @see GetProxies, FindNewContacts.
    void AddProxies(const Proxies& proxies);

    /// @brief Finds new contacts.
    /// @details Processes the proxy queue for finding new contacts and adding them to
    ///   the contacts container.
    /// @note New contacts will all have overlapping AABBs.
    /// @post <code>GetProxies()</code> will return an empty container.
    /// @see GetProxies.
    ContactCounter FindNewContacts();

    /// @brief Gets the fixtures-for-proxies range for this world.
    /// @details Provides insight on what fixtures have been queued for proxy processing
    ///   during the next call to the world step method.
    /// @see Step.
    std::vector<std::pair<BodyID, ShapeID>> GetFixturesForProxies() const noexcept;

    /// @brief Determines whether this world has new fixtures.
    bool HasNewFixtures() const noexcept;

    /// @}

    /// @name Body Member Functions
    /// Member functions relating to bodies.
    /// @{

    /// @brief Gets the extent of the currently valid body range.
    /// @note This is one higher than the maxium <code>BodyID</code> that is in range
    ///   for body related functions.
    BodyCounter GetBodyRange() const noexcept;

    /// @brief Gets the world body range for this constant world.
    /// @details Gets a range enumerating the bodies currently existing within this world.
    ///   These are the bodies that had been created from previous calls to the
    ///   <code>CreateBody(const Body&)</code> method that haven't yet been destroyed.
    /// @return Body range that can be iterated over using its begin and end methods
    ///   or using ranged-based for-loops.
    /// @see CreateBody(const Body&).
    Bodies GetBodies() const noexcept;

    /// @brief Gets the bodies-for-proxies range for this world.
    /// @details Provides insight on what bodies have been queued for proxy processing
    ///   during the next call to the world step method.
    /// @see Step.
    Bodies GetBodiesForProxies() const noexcept;

    /// @brief Creates a rigid body that's a copy of the given one.
    /// @warning This function should not be used while the world is locked &mdash; as it is
    ///   during callbacks. If it is, it will throw an exception or abort your program.
    /// @note No references to the configuration are retained. Its value is copied.
    /// @post The created body will be present in the range returned from the
    ///   <code>GetBodies()</code> method.
    /// @param body A customized body or its default value.
    /// @return Identifier of the newly created body which can later be destroyed by calling
    ///   the <code>Destroy(BodyID)</code> method.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws LengthError if this operation would create more than <code>MaxBodies</code>.
    /// @throws std::out_of_range if the given body references any invalid shape identifiers.
    /// @see Destroy(BodyID), GetBodies.
    /// @see PhysicalEntities.
    BodyID CreateBody(Body body);

    /// @brief Gets the identified body.
    /// @throws std::out_of_range if given an invalid id.
    /// @see SetBody, GetBodyRange.
    const Body& GetBody(BodyID id) const;

    /// @brief Sets the identified body.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws std::out_of_range if given an invalid id of if the given body references any
    ///   invalid shape identifiers.
    /// @throws InvalidArgument if the specified ID was destroyed.
    /// @see GetBody, GetBodyRange.
    void SetBody(BodyID id, Body value);

    /// @brief Destroys the identified body.
    /// @details Destroys a given body that had previously been created by a call to this
    ///   world's <code>CreateBody(const Body&)</code> method.
    /// @warning This automatically deletes all associated shapes and joints.
    /// @warning This function is locked during callbacks.
    /// @warning Behavior is undefined if given a null body.
    /// @warning Behavior is undefined if the passed body was not created by this world.
    /// @note This function is locked during callbacks.
    /// @post The destroyed body will no longer be present in the range returned from the
    ///   <code>GetBodies()</code> method.
    /// @post None of the body's fixtures will be present in the fixtures-for-proxies
    ///   collection.
    /// @param id Body to destroy that had been created by this world.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws std::out_of_range If given an invalid body identifier.
    /// @see CreateBody(const Body&), GetBodies, GetFixturesForProxies.
    /// @see PhysicalEntities.
    void Destroy(BodyID id);

    /// @brief Gets whether the given identifier is to a body that's been destroyed.
    /// @note Complexity is at most O(n) where n is the number of elements free.
    bool IsDestroyed(BodyID id) const noexcept;

    /// @brief Gets the proxies for the identified body.
    /// @throws std::out_of_range If given an invalid identifier.
    const Proxies& GetProxies(BodyID id) const;

    /// @brief Gets the contacts associated with the identified body.
    /// @throws std::out_of_range if given an invalid id.
    Contacts GetContacts(BodyID id) const;

    /// @throws std::out_of_range if given an invalid id.
    BodyJoints GetJoints(BodyID id) const;

    /// @}

    /// @name Joint Member Functions
    /// Member functions relating to joints.
    /// @{

    /// @brief Gets the extent of the currently valid joint range.
    /// @note This is one higher than the maxium <code>JointID</code> that is in range
    ///   for joint related functions.
    JointCounter GetJointRange() const noexcept;

    /// @brief Gets the world joint range.
    /// @details Gets a range enumerating the joints currently existing within this world.
    ///   These are the joints that had been created from previous calls to the
    ///   <code>CreateJoint(const Joint&)</code> method that haven't yet been destroyed.
    /// @return World joints sized-range.
    /// @see CreateJoint(const Joint&).
    Joints GetJoints() const noexcept;

    /// @brief Creates a joint to constrain one or more bodies.
    /// @warning This function is locked during callbacks.
    /// @note No references to the configuration are retained. Its value is copied.
    /// @post The created joint will be present in the range returned from the
    ///   <code>GetJoints()</code> method.
    /// @return Identifier for the newly created joint which can later be destroyed by calling
    ///   the <code>Destroy(JointID)</code> method.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws LengthError if this operation would create more than <code>MaxJoints</code>.
    /// @throws InvalidArgument if the given definition is not allowed.
    /// @throws std::out_of_range if the given joint references any invalid body id.
    /// @see PhysicalEntities.
    /// @see Destroy(JointID), GetJoints.
    JointID CreateJoint(Joint def);

    /// @brief Gets the identified joint.
    /// @throws std::out_of_range if given an invalid ID.
    const Joint& GetJoint(JointID id) const;

    /// @brief Sets the identified joint.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws std::out_of_range if given an invalid ID or the given joint references any
    ///    invalid body ID.
    /// @throws InvalidArgument if the specified ID was destroyed.
    /// @see CreateJoint(Joint def), Destroy(JointID joint).
    void SetJoint(JointID id, Joint def);

    /// @brief Destroys a joint.
    /// @details Destroys a given joint that had previously been created by a call to this
    ///   world's <code>CreateJoint(const Joint&)</code> method.
    /// @warning This function is locked during callbacks.
    /// @warning Behavior is undefined if the passed joint was not created by this world.
    /// @note This may cause the connected bodies to begin colliding.
    /// @post The destroyed joint will no longer be present in the range returned from the
    ///   <code>GetJoints()</code> method.
    /// @param joint Joint to destroy that had been created by this world.
    /// @throws WrongState if this function is called while the world is locked.
    /// @see CreateJoint(const Joint&), GetJoints.
    /// @see PhysicalEntities.
    void Destroy(JointID joint);

    /// @brief Gets whether the given identifier is to a joint that's been destroyed.
    /// @note Complexity is at most O(n) where n is the number of elements free.
    bool IsDestroyed(JointID id) const noexcept;

    /// @}

    /// @name Shape Member Functions
    /// Member functions relating to shapes.
    /// @{

    /// @brief Gets the extent of the currently valid shape range.
    /// @note This is one higher than the maxium <code>ShapeID</code> that is in range
    ///   for shape related functions.
    ShapeCounter GetShapeRange() const noexcept;

    /// @brief Creates an identifiable copy of the given shape within this world.
    /// @throws InvalidArgument if called for a shape with a vertex radius that's either:
    ///    less than the minimum vertex radius, or greater than the maximum vertex radius.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws LengthError if this operation would create more than <code>MaxShapes</code>.
    /// @see Destroy(ShapeID), GetShape, SetShape.
    ShapeID CreateShape(Shape def);

    /// @brief Gets the identified shape.
    /// @throws std::out_of_range If given an invalid shape identifier.
    /// @see CreateShape.
    const Shape& GetShape(ShapeID id) const;

    /// @brief Sets the value of the identified shape.
    /// @warning This function is locked during callbacks.
    /// @note This function does not reset the mass data of any effected bodies.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws std::out_of_range If given an invalid identifier.
    /// @throws InvalidArgument if the specified ID was destroyed.
    /// @see CreateShape, Destroy(ShapeID id).
    void SetShape(ShapeID id, Shape def);

    /// @brief Destroys the identified shape removing any body associations with it first.
    /// @warning This function is locked during callbacks.
    /// @note This function does not reset the mass data of any effected bodies.
    /// @throws WrongState if this function is called while the world is locked.
    /// @throws std::out_of_range If given an invalid shape identifier.
    /// @see CreateShape, Detach.
    void Destroy(ShapeID id);

    /// @}

    /// @name Contact Member Functions
    /// Member functions relating to contacts.
    /// @{

    /// @brief Gets the extent of the currently valid contact range.
    /// @note This is one higher than the maxium <code>ContactID</code> that is in range
    ///   for contact related functions.
    ContactCounter GetContactRange() const noexcept;

    /// @brief Gets the world contact range.
    /// @warning contacts are created and destroyed in the middle of a time step.
    /// Use <code>ContactListener</code> to avoid missing contacts.
    /// @return World contacts sized-range.
    Contacts GetContacts() const noexcept;

    /// @brief Gets the identified contact.
    /// @throws std::out_of_range If given an invalid contact identifier.
    /// @see SetContact.
    const Contact& GetContact(ContactID id) const;

    /// @brief Sets the identified contact's state.
    /// @note This will throw an exception to preserve invariants.
    /// @invariant A contact may only be impenetrable if one or both bodies are.
    /// @invariant A contact may only be active if one or both bodies are awake.
    /// @invariant A contact may only be a sensor or one or both shapes are.
    /// @throws InvalidArgument if a change would violate an invariant or if the specified ID
    ///   was destroyed.
    /// @throws std::out_of_range If given an invalid contact identifier or an invalid identifier
    ///   in the new contact value.
    /// @see GetContact.
    void SetContact(ContactID id, Contact value);

    /// @brief Gets the identified manifold.
    /// @throws std::out_of_range If given an invalid contact identifier.
    const Manifold& GetManifold(ContactID id) const;

    /// @brief Gets whether the given identifier is to a contact that's been destroyed.
    /// @note Complexity is at most O(n) where n is the number of elements free.
    bool IsDestroyed(ContactID id) const noexcept;

    /// @}

private:
    /// @brief Flags type data type.
    using FlagsType = std::uint32_t;

    /// @brief Flag enumeration.
    enum Flag: FlagsType
    {
        /// New fixture.
        e_newFixture = 0x0001,

        /// Locked.
        e_locked = 0x0002,

        /// Sub-stepping.
        e_substepping = 0x0020,

        /// Step complete. @details Used for sub-stepping. @see e_substepping.
        e_stepComplete = 0x0040,

        /// Needs contact filtering.
        e_needsContactFiltering = 0x0080,
    };

    /// @brief Contact key queue type alias.
    using ContactKeyQueue = std::vector<ContactKey>;

    /// @brief Solves the step.
    /// @details Finds islands, integrates and solves constraints, solves position constraints.
    /// @note This may miss collisions involving fast moving bodies and allow them to tunnel
    ///   through each other.
    RegStepStats SolveReg(const StepConf& conf);

    /// @brief Solves the given island (regularly).
    ///
    /// @details This:
    ///   1. Updates every island-body's <code>sweep.pos0</code> to its <code>sweep.pos1</code>.
    ///   2. Updates every island-body's <code>sweep.pos1</code> to the new normalized "solved"
    ///      position for it.
    ///   3. Updates every island-body's velocity to the new accelerated, dampened, and "solved"
    ///      velocity for it.
    ///   4. Synchronizes every island-body's transform (by updating it to transform one of the
    ///      body's sweep).
    ///   5. Reports to the listener (if non-null).
    ///
    /// @param conf Time step configuration information.
    /// @param island Island of bodies, contacts, and joints to solve for. Must contain at least
    ///   one body, contact, or joint.
    ///
    /// @warning Behavior is undefined if the given island doesn't have at least one body,
    ///   contact, or joint.
    ///
    /// @return Island solver results.
    ///
    IslandStats SolveRegIslandViaGS(const StepConf& conf, const Island& island);
    
    /// @brief Adds to the island based off of a given "seed" body.
    /// @post Contacts are listed in the island in the order that bodies provide those contacts.
    /// @post Joints are listed the island in the order that bodies provide those joints.
    void AddToIsland(Island& island, BodyID seed,
                     BodyCounter& remNumBodies,
                     ContactCounter& remNumContacts,
                     JointCounter& remNumJoints);
    
    /// @brief Body stack.
    using BodyStack = std::stack<BodyID, std::vector<BodyID>>;
    
    /// @brief Adds to the island.
    void AddToIsland(Island& island, BodyStack& stack,
                     BodyCounter& remNumBodies,
                     ContactCounter& remNumContacts,
                     JointCounter& remNumJoints);
    
    /// @brief Adds contacts to the island.
    void AddContactsToIsland(Island& island, BodyStack& stack, const Contacts& contacts,
                             BodyID bodyID);

    /// @brief Adds joints to the island.
    void AddJointsToIsland(Island& island, BodyStack& stack, const BodyJoints& joints);
    
    /// @brief Removes <em>unspeedables</em> from the is <em>is-in-island</em> state.
    static Bodies::size_type RemoveUnspeedablesFromIslanded(const std::vector<BodyID>& bodies,
                                                            const ArrayAllocator<Body>& buffer,
                                                            std::vector<bool>& islanded);
    
    /// @brief Solves the step using successive time of impact (TOI) events.
    /// @details Used for continuous physics.
    /// @note This is intended to detect and prevent the tunneling that the faster Solve method
    ///    may miss.
    /// @param conf Time step configuration to use.
    ToiStepStats SolveToi(const StepConf& conf);

    /// @brief Solves collisions for the given time of impact.
    ///
    /// @param contactID Identifier of contact to solve for.
    /// @param conf Time step configuration to solve for.
    ///
    /// @note Precondition 1: there is no contact having a lower TOI in this time step that has
    ///   not already been solved for.
    /// @note Precondition 2: there is not a lower TOI in the time step for which collisions have
    ///   not already been processed.
    ///
    IslandStats SolveToi(ContactID contactID, const StepConf& conf);

    /// @brief Solves the time of impact for bodies 0 and 1 of the given island.
    ///
    /// @details This:
    ///   1. Updates position 0 of the sweeps of bodies 0 and 1.
    ///   2. Updates position 1 of the sweeps, the transforms, and the velocities of the other
    ///      bodies in this island.
    ///
    /// @pre <code>island.bodies</code> contains at least two bodies, the first two of which
    ///   are bodies 0 and 1.
    /// @pre <code>island.bodies</code> contains appropriate other bodies of the contacts of
    ///   the two bodies.
    /// @pre <code>island.contacts</code> contains the contact that specified the two identified
    ///   bodies.
    /// @pre <code>island.contacts</code> contains appropriate other contacts of the two bodies.
    ///
    /// @param conf Time step configuration information.
    /// @param island Island to do time of impact solving for.
    ///
    /// @return Island solver results.
    ///
    IslandStats SolveToiViaGS(const Island& island, const StepConf& conf);

    /// @brief Process contacts output.
    struct ProcessContactsOutput
    {
        ContactCounter contactsUpdated = 0; ///< Contacts updated.
        ContactCounter contactsSkipped = 0; ///< Contacts skipped.
    };

    /// @brief Processes the contacts of a given body for TOI handling.
    /// @details This does the following:
    ///   1. Advances the appropriate associated other bodies to the given TOI (advancing
    ///      their sweeps and synchronizing their transforms to their new sweeps).
    ///   2. Updates the contact manifolds and touching statuses and notifies listener (if one given) of
    ///      the appropriate contacts of the body.
    ///   3. Adds those contacts that are still enabled and still touching to the given island
    ///      (or resets the other bodies advancement).
    ///   4. Adds to the island, those other bodies that haven't already been added of the contacts that got added.
    /// @note Precondition: there should be no lower TOI for which contacts have not already been processed.
    /// @param[in,out] id Identifier of the dynamic/accelerable body to process contacts for.
    /// @param[in,out] island Island. On return this may contain additional contacts or bodies.
    /// @param[in] toi Time of impact (TOI). Value between 0 and 1.
    /// @param[in] conf Step configuration data.
    ProcessContactsOutput ProcessContactsForTOI(BodyID id, Island& island, Real toi,
                                                const StepConf& conf);

    /// @brief Removes the given body from this world.
    void Remove(BodyID id) noexcept;

    /// @brief Updates associated bodies and contacts for specified joint's addition.
    void Add(JointID j, bool flagForFiltering = false);

    /// @brief Updates associated bodies and contacts for specified joint's removal.
    void Remove(JointID id) noexcept;

    /// @brief Sets the step complete state.
    /// @post <code>IsStepComplete()</code> will return the value set.
    /// @see IsStepComplete.
    void SetStepComplete(bool value) noexcept;

    /// @brief Sets the allow sleeping state.
    void SetAllowSleeping() noexcept;

    /// @brief Unsets the allow sleeping state.
    void UnsetAllowSleeping() noexcept;

    /// @brief Update contacts statistics.
    struct UpdateContactsStats
    {
        /// @brief Number of contacts ignored (because both bodies were asleep).
        ContactCounter ignored = 0;

        /// @brief Number of contacts updated.
        ContactCounter updated = 0;

        /// @brief Number of contacts skipped because they weren't marked as needing updating.
        ContactCounter skipped = 0;
    };

    /// @brief Destroy contacts statistics.
    struct DestroyContactsStats
    {
        ContactCounter overlap = 0; ///< Erased by not overlapping.
        ContactCounter filter = 0; ///< Erased due to filtering.
    };

    /// @brief Contact TOI data.
    struct ContactToiData
    {
        ContactID contact = InvalidContactID; ///< Contact for which the time of impact is relevant.
        Real toi = std::numeric_limits<Real>::infinity(); ///< Time of impact (TOI) as a fractional value between 0 and 1.
        ContactCounter simultaneous = 0; ///< Count of simultaneous contacts at this TOI.
    };

    /// @brief Update contacts data.
    struct UpdateContactsData
    {
        ContactCounter numAtMaxSubSteps = 0; ///< # at max sub-steps (lower the better).
        ContactCounter numUpdatedTOI = 0; ///< # updated TOIs (made valid).
        ContactCounter numValidTOI = 0; ///< # already valid TOIs.

        /// @brief Distance iterations type alias.
        using dist_iter_type = std::remove_const<decltype(DefaultMaxDistanceIters)>::type;

        /// @brief TOI iterations type alias.
        using toi_iter_type = std::remove_const<decltype(DefaultMaxToiIters)>::type;

        /// @brief Root iterations type alias.
        using root_iter_type = std::remove_const<decltype(DefaultMaxToiRootIters)>::type;

        dist_iter_type maxDistIters = 0; ///< Max distance iterations.
        toi_iter_type maxToiIters = 0; ///< Max TOI iterations.
        root_iter_type maxRootIters = 0; ///< Max root iterations.
    };

    /// @brief Updates the contact times of impact.
    UpdateContactsData UpdateContactTOIs(const StepConf& conf);

    /// @brief Gets the soonest contact.
    /// @details This finds the contact with the lowest (soonest) time of impact.
    /// @return Contact with the least time of impact and its time of impact, or null contact.
    ///  A non-null contact will be enabled, not have sensors, be active, and impenetrable.
    static ContactToiData GetSoonestContact(const Contacts& contacts,
                                            const ArrayAllocator<Contact>& buffer) noexcept;

    /// @brief Unsets the new fixtures state.
    void UnsetNewFixtures() noexcept;

    /// @brief Processes the narrow phase collision for the contacts collection.
    /// @details
    /// This finds and destroys the contacts that need filtering and no longer should collide or
    /// that no longer have AABB-based overlapping fixtures. Those contacts that persist and
    /// have active bodies (either or both) get their Update methods called with the current
    /// contact listener as its argument.
    /// Essentially this really just purges contacts that are no longer relevant.
    DestroyContactsStats DestroyContacts(Contacts& contacts);
    
    /// @brief Update contacts.
    UpdateContactsStats UpdateContacts(const StepConf& conf);

    /// @brief Destroys the given contact and removes it from its container.
    /// @details This updates the contacts container, returns the memory to the allocator,
    ///   and decrements the contact manager's contact count.
    /// @param contact Contact to destroy.
    /// @param from From body.
    void Destroy(ContactID contact, const Body* from);

    /// @brief Adds a contact for the proxies identified by the key if appropriate.
    /// @details Adds a new contact object to represent a contact between proxy A and proxy B
    /// if all of the following are true:
    ///   1. The bodies of the fixtures of the proxies are not the one and the same.
    ///   2. No contact already exists for these two proxies.
    ///   3. The bodies of the proxies should collide (according to <code>ShouldCollide</code>).
    ///   4. The contact filter says the fixtures of the proxies should collide.
    ///   5. There exists a contact-create function for the pair of shapes of the proxies.
    /// @post The size of the <code>contacts</code> collection is one greater-than it was
    ///   before this method is called if it returns <code>true</code>.
    /// @param key ID's of dynamic tree entries identifying the fixture proxies involved.
    /// @return <code>true</code> if a new contact was indeed added (and created),
    ///   else <code>false</code>.
    /// @see bool ShouldCollide(const Body& lhs, const Body& rhs) noexcept.
    bool Add(ContactKey key);

    /// @brief Destroys the given contact.
    void InternalDestroy(ContactID contact, const Body* from = nullptr);

    /// @brief Synchronizes the given body.
    /// @details This updates the broad phase dynamic tree data for all of the identified shapes.
    ContactCounter Synchronize(BodyID bodyId,
                               const Transformation& xfm1, const Transformation& xfm2,
                               Real multiplier, Length extension);

    /// @brief Updates the touching related state and notifies listener (if one given).
    ///
    /// @note Ideally this method is only called when a dependent change has occurred.
    /// @note Touching related state depends on the following data:
    ///   - The fixtures' sensor states.
    ///   - The fixtures bodies' transformations.
    ///   - The <code>maxCirclesRatio</code> per-step configuration state *OR* the
    ///     <code>maxDistanceIters</code> per-step configuration state.
    ///
    /// @param id Identifies the contact to update.
    /// @param conf Per-step configuration information.
    ///
    /// @see GetManifold, IsTouching
    ///
    void Update(ContactID id, const ContactUpdateConf& conf);

    /******** Member variables. ********/

    DynamicTree m_tree; ///< Dynamic tree.

    ArrayAllocator<Body> m_bodyBuffer; ///< Array of body data both used and freed.
    ArrayAllocator<Shape> m_shapeBuffer; ///< Array of shape data both used and freed.
    ArrayAllocator<Joint> m_jointBuffer; ///< Array of joint data both used and freed.
    ArrayAllocator<Contact> m_contactBuffer; ///< Array of contact data both used and freed.
    ArrayAllocator<Manifold> m_manifoldBuffer; ///< Array of manifold data both used and freed.

    ArrayAllocator<Contacts> m_bodyContacts; ///< Cache of contacts associated with bodies.
    ArrayAllocator<BodyJoints> m_bodyJoints; ///< Cache of joints associated with bodies.
    ArrayAllocator<Proxies> m_bodyProxies; ///< Cache of proxies associated with bodies.

    ContactKeyQueue m_proxyKeys; ///< Proxy keys.
    Proxies m_proxiesForContacts; ///< Proxies queue.
    std::vector<std::pair<BodyID, ShapeID>> m_fixturesForProxies; ///< Fixtures for proxies queue.
    std::vector<BodyConstraint> m_bodyConstraints; ///< Cache of constraints associated with bodies.
    Bodies m_bodiesForSync; ///< Bodies for proxies queue.

    Bodies m_bodies; ///< Body collection.

    Joints m_joints; ///< Joint collection.
    
    /// @brief Container of contacts.
    /// @note In the <em>add pair</em> stress-test, 401 bodies can have some 31000 contacts
    ///   during a given time step.
    Contacts m_contacts;

    Island m_island; ///< Island buffer.
    std::vector<bool> m_islandedBodies; ///< Per body boolean on whether body islanded.
    std::vector<bool> m_islandedContacts; ///< Per contact boolean on whether contact islanded.
    std::vector<bool> m_islandedJoints; ///< Per joint boolean on whether joint islanded.

    ShapeListener m_shapeDestructionListener; ///< Listener for shape destruction.
    AssociationListener m_detachListener; ///< Listener for shapes detaching from bodies.
    JointListener m_jointDestructionListener; ///< Listener for joint destruction.
    ContactListener m_beginContactListener; ///< Listener for beginning contact events.
    ContactListener m_endContactListener; ///< Listener for ending contact events.
    ManifoldContactListener m_preSolveContactListener; ///< Listener for pre-solving contacts.
    ImpulsesContactListener m_postSolveContactListener; ///< Listener for post-solving contacts.

    FlagsType m_flags = e_stepComplete; ///< Flags.
    
    /// Inverse delta-t from previous step.
    /// @details Used to compute time step ratio to support a variable time step.
    /// @note 4-bytes large.
    /// @see Step.
    Frequency m_inv_dt0 = 0;
    
    /// @brief Minimum vertex radius.
    Positive<Length> m_minVertexRadius;
    
    /// @brief Maximum vertex radius.
    /// @details
    /// This is the maximum shape vertex radius that any bodies' of this world should create
    /// fixtures for. Requests to create fixtures for shapes with vertex radiuses bigger than
    /// this must be rejected. As an upper bound, this value prevents shapes from getting
    /// associated with this world that would otherwise not be able to be simulated due to
    /// numerical issues. It can also be set below this upper bound to constrain the differences
    /// between shape vertex radiuses to possibly more limited visual ranges.
    Positive<Length> m_maxVertexRadius;
};

inline const WorldImpl::Proxies& WorldImpl::GetProxies() const noexcept
{
    return m_proxiesForContacts;
}

inline void WorldImpl::AddProxies(const Proxies& proxies)
{
    m_proxiesForContacts.insert(end(m_proxiesForContacts), begin(proxies), end(proxies));
}

inline WorldImpl::Bodies WorldImpl::GetBodies() const noexcept
{
    return m_bodies;
}

inline WorldImpl::Bodies WorldImpl::GetBodiesForProxies() const noexcept
{
    return m_bodiesForSync;
}

inline std::vector<std::pair<BodyID, ShapeID>> WorldImpl::GetFixturesForProxies() const noexcept
{
    return m_fixturesForProxies;
}

inline WorldImpl::Joints WorldImpl::GetJoints() const noexcept
{
    return m_joints;
}

inline WorldImpl::Contacts WorldImpl::GetContacts() const noexcept
{
    return m_contacts;
}

inline bool WorldImpl::IsLocked() const noexcept
{
    return (m_flags & e_locked) == e_locked;
}

inline bool WorldImpl::IsStepComplete() const noexcept
{
    return (m_flags & e_stepComplete) != 0u;
}

inline void WorldImpl::SetStepComplete(bool value) noexcept
{
    if (value) {
        m_flags |= e_stepComplete;
    }
    else {
        m_flags &= ~e_stepComplete;        
    }
}

inline bool WorldImpl::GetSubStepping() const noexcept
{
    return (m_flags & e_substepping) != 0u;
}

inline void WorldImpl::SetSubStepping(bool flag) noexcept
{
    if (flag) {
        m_flags |= e_substepping;
    }
    else {
        m_flags &= ~e_substepping;
    }
}

inline bool WorldImpl::HasNewFixtures() const noexcept
{
    return (m_flags & e_newFixture) != 0u;
}

inline void WorldImpl::UnsetNewFixtures() noexcept
{
    m_flags &= ~e_newFixture;
}

inline Length WorldImpl::GetMinVertexRadius() const noexcept
{
    return m_minVertexRadius;
}

inline Length WorldImpl::GetMaxVertexRadius() const noexcept
{
    return m_maxVertexRadius;
}

inline Frequency WorldImpl::GetInvDeltaTime() const noexcept
{
    return m_inv_dt0;
}

inline const DynamicTree& WorldImpl::GetTree() const noexcept
{
    return m_tree;
}

inline void WorldImpl::SetShapeDestructionListener(ShapeListener listener) noexcept
{
    m_shapeDestructionListener = std::move(listener);
}

inline void WorldImpl::SetDetachListener(AssociationListener listener) noexcept
{
    m_detachListener = std::move(listener);
}

inline void WorldImpl::SetJointDestructionListener(JointListener listener) noexcept
{
    m_jointDestructionListener = std::move(listener);
}

inline void WorldImpl::SetBeginContactListener(ContactListener listener) noexcept
{
    m_beginContactListener = std::move(listener);
}

inline void WorldImpl::SetEndContactListener(ContactListener listener) noexcept
{
    m_endContactListener = std::move(listener);
}

inline void WorldImpl::SetPreSolveContactListener(ManifoldContactListener listener) noexcept
{
    m_preSolveContactListener = std::move(listener);
}

inline void WorldImpl::SetPostSolveContactListener(ImpulsesContactListener listener) noexcept
{
    m_postSolveContactListener = std::move(listener);
}

} // namespace d2
} // namespace playrho

#endif // PLAYRHO_DYNAMICS_WORLDIMPL_HPP
