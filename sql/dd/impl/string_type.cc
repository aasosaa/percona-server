/* Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "dd/string_type.h"

#include "my_sys.h"                     // MY_WME, MY_FATALERROR
#include "mysql/service_mysql_alloc.h"  // my_malloc
#include "my_murmur3.h"                 // my_murmur3_32
#include "psi_memory_key.h"             // key_memory_DD_String_type


namespace dd {
void *String_type_alloc::operator()(size_t s) const
{
  return my_malloc(key_memory_DD_String_type, s,
                   MYF(MY_WME | ME_FATALERROR));
}
}

namespace std {
// TODO: dd::String_type_alias -> dd::String_type
size_t hash<dd::String_type_alias>::operator()(const dd::String_type_alias &s)
  const
{
  return murmur3_32(reinterpret_cast<const uchar*>(s.c_str()), s.size(), 0);
}
}
