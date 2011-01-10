/* This file is part of Machina.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * Machina is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Machina is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Machina.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MACHINA_UPDATES_HPP
#define MACHINA_UPDATES_HPP

#include <stdint.h>

#include "raul/Atom.hpp"
#include "raul/SharedPtr.hpp"

#include "machina/types.hpp"

namespace Machina {

enum UpdateType {
	UPDATE_SET = 1
};

void
write_set(SharedPtr<UpdateBuffer> buf,
          uint64_t                subject,
          URIInt                  key,
          const Raul::Atom&       value);

uint32_t
read_set(SharedPtr<UpdateBuffer> buf,
         uint64_t*               subject,
         URIInt*                 key,
         Raul::Atom*             value);

} // namespace Machina

#endif // MACHINA_UPDATES_HPP
