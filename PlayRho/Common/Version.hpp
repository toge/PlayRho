/*
 * Original work Copyright (c) 2006-2009 Erin Catto http://www.box2d.org
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

#ifndef PLAYRHO_COMMON_VERSION_HPP
#define PLAYRHO_COMMON_VERSION_HPP

#include <cstdint>
#include <string>

namespace playrho {
    
    /// Version numbering scheme.
    /// See http://en.wikipedia.org/wiki/Software_versioning
    struct Version
    {
        /// @brief Revision number type.
        using revnum_type = std::int32_t;
        
        revnum_type major; ///< significant changes
        revnum_type minor; ///< incremental changes
        revnum_type revision; ///< bug fixes
    };
    
    /// @brief Equality operator.
    constexpr inline bool operator== (Version lhs, Version rhs)
    {
        return lhs.major == rhs.major && lhs.minor == rhs.minor && lhs.revision == rhs.revision;
    }
    
    /// @brief Inequality operator.
    constexpr inline bool operator!= (Version lhs, Version rhs)
    {
        return !(lhs == rhs);
    }
    
    /// @brief Gets the version information of the library.
    Version GetVersion() noexcept;
    
    /// @brief Gets the build details of the library.
    std::string GetBuildDetails() noexcept;

} // namespace playrho

#endif // PLAYRHO_COMMON_VERSION_HPP