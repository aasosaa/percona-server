/* Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.

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

#ifndef DD__TRANSACTION_IMPL_INCLUDED
#define DD__TRANSACTION_IMPL_INCLUDED

#include <sys/types.h>
#include <map>

#include "dd/dd_kill_immunizer.h"     // dd::DD_kill_immunizer
#include "dd/string_type.h"           // dd::String_type
#include "discrete_interval.h"
#include "field.h"
#include "handler.h"
#include "my_inttypes.h"
#include "mysql/udf_registration_types.h"
#include "set_var.h"
#include "sql_class.h"                // THD::killed_state
#include "thr_lock.h"

struct LEX;

namespace dd {

class Raw_table;

///////////////////////////////////////////////////////////////////////////

/**
  Auxiliary class for opening dictionary tables.
*/
class Open_dictionary_tables_ctx
{
public:
  Open_dictionary_tables_ctx(THD *thd, thr_lock_type lock_type )
    : m_thd(thd),
      m_lock_type(lock_type),
      m_ignore_global_read_lock(false)
  { }

  ~Open_dictionary_tables_ctx();

  Raw_table *get_table(const String_type &name) const;

  template <typename T>
  Raw_table *get_table() const
  { return get_table(T::OBJECT_TABLE().name()); }

  template <typename X>
  void register_tables()
  { X::TYPE().register_tables(this); }

  template <typename X>
  void add_table()
  { this->add_table(X::table_name()); }

  /**
    Open all the DD tables in list Open_dictionary_tables_ctx::m_tables.

    @return true - On failure and error is reported.
    @return false - On success.
  */
  bool open_tables();

  void add_table(const String_type &name);

  THD *get_thd() const { return m_thd; }

  /**
    Ignore global read lock when opening the tables.

    @returns void.
  */
  void mark_ignore_global_read_lock()
  {
    if (m_lock_type == TL_WRITE)
      m_ignore_global_read_lock= true;
  }

private:
  THD *m_thd;
  thr_lock_type m_lock_type;
  bool m_ignore_global_read_lock;
  typedef std::map<String_type, Raw_table *> Object_table_map;
  Object_table_map m_tables;
};


/**
  Implementation of read-only data-dictionary transaction.
  Relies on attachable transactions infrastructure.
*/
class Transaction_ro
{
public:
  Transaction_ro(THD *thd, enum_tx_isolation isolation)
   : otx(thd, TL_READ),
     m_thd(thd),
     m_kill_immunizer(thd)
  {
    thd->begin_attachable_ro_transaction();
    thd->tx_isolation= isolation;
  }

  ~Transaction_ro()
  {
    m_thd->end_attachable_transaction();
  }

  Open_dictionary_tables_ctx otx;

private:
  THD *m_thd;

  DD_kill_immunizer m_kill_immunizer;
};


///////////////////////////////////////////////////////////////////////////

/**
  Class for storing/restoring state during dictionary update operations.
*/
class Update_dictionary_tables_ctx
{
public:
  Update_dictionary_tables_ctx(THD *thd);

  ~Update_dictionary_tables_ctx();

  Open_dictionary_tables_ctx otx;

private:
  THD *m_thd;

  DD_kill_immunizer m_kill_immunizer;

  LEX *m_lex_saved;

  // Stores state before DD operations
  Open_tables_backup m_open_tables_state_backup;
  bool m_saved_binlog_row_based;
  ulonglong m_saved_binlog_options;
  sql_mode_t m_saved_mode;
  long long m_latest_auto_incr_id;
  enum_check_fields m_saved_check_for_truncated_fields;
  uint m_saved_in_sub_stmt;
  bool m_saved_time_zone_used;

  /*
    Save auto_increment_increment state.

    Value of the auto_increment_increment is set to 1 in the constructor.
    auto_increment_offset is used only when auto_increment_increment is greater
    than 1. So saving only auto_increment_increment value.
  */
  ulong m_saved_auto_increment_increment;

  // Save auto_inc_intervals_in_cur_stmt_for_binlog
  Discrete_intervals_list m_auto_inc_intervals_in_cur_stmt_for_binlog_saved;

  Discrete_intervals_list m_auto_inc_intervals_forced_saved;
};

///////////////////////////////////////////////////////////////////////////

}

#endif // DD__TRANSACTION_IMPL_INCLUDED
