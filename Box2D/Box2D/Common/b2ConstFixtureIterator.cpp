//
//  ConstFixtureIterator.cpp
//  Box2D
//
//  Created by Louis D. Langholtz on 3/7/16.
//
//

#include <Box2D/Common/b2ConstFixtureIterator.hpp>
#include <Box2D/Dynamics/b2Fixture.h>

using namespace box2d;

ConstFixtureIterator::pointer ConstFixtureIterator::next(pointer q) const noexcept
{
	return p->GetNext();
}