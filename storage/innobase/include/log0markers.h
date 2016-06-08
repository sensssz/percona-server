/*****************************************************************************

Copyright (c) 2016, Percona Inc. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
Street, Fifth Floor, Boston, MA 02110-1301, USA

*****************************************************************************/

/**************************************************//**
@file include/log0markers.h
Selected redo log record offset tracking for parallel recovery
*******************************************************/

#ifndef log0markers_h
#define log0markers_h

#include <vector>
#include <utility>

#include "db0err.h"	// dberr_t
#include "log0types.h"	// lsn_t

typedef std::pair<lsn_t, lsn_t> lsn_range_t;

MY_ATTRIBUTE((warn_unused_result))
dberr_t
log_markers_create(void);

void
log_markers_close(void);

void
log_markers_maybe_mark(lsn_t lsn);

MY_ATTRIBUTE((warn_unused_result))
std::vector<lsn_range_t>
log_markers_get(lsn_t checkpoint_lsn, lsn_t log_end_lsn,
		dberr_t &err);

#endif
