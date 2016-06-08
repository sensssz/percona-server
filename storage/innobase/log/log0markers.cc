/*****************************************************************************

Copyright (c) 2011-2012 Percona Inc. All Rights Reserved.

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
@file log/log0markers.cc
Selected redo log record offset tracking for parallel recovery
*******************************************************/


#include "log0markers.h"

#include "os0file.h"
#include "srv0srv.h"

static char		log_marker_path[FN_REFLEN] = "";

static os_file_t	log_marker_file = OS_FILE_CLOSED;

static os_offset_t	log_marker_file_offset = 0;

static lsn_t		last_marked_lsn = 0;

#define LSN_MARKING_STEP (50 * 1024 /* * 1024 */)

// TODO
#define LSN_MARKER_PARTITION_COUNT 4

// TODO
#define log_marker_name "xb_log_markers"

#ifdef UNIV_PFS_MUTEX
mysql_pfs_key_t	log_marker_sys_mutex_key;
#endif /* UNIV_PFS_MUTEX */

ib_mutex_t	log_marker_sys_mutex;

static
void
log_markers_make_path(void)
{
	// TODO: copy paste from log_online_open_bitmap_file_read_only
	size_t  srv_data_home_len = strlen(srv_data_home);
	if (srv_data_home_len
	    && srv_data_home[srv_data_home_len-1]
	    != SRV_PATH_SEPARATOR) {
		ut_snprintf(log_marker_path, FN_REFLEN, "%s%c%s",
			    srv_data_home, SRV_PATH_SEPARATOR,
			    log_marker_name);
	} else {
		ut_snprintf(log_marker_path, FN_REFLEN, "%s%s",
			    srv_data_home, log_marker_name);
	}
}

dberr_t
log_markers_create(void)
{
	bool success;

	log_markers_make_path();

	mutex_create(LATCH_ID_LOG_MARKER_SYS, &log_marker_sys_mutex);

	log_marker_file
		= os_file_create_simple(PSI_NOT_INSTRUMENTED, // TODO
					log_marker_path,
					OS_FILE_CREATE,
					OS_FILE_READ_WRITE,
					false, &success);

	if (!success) {
		char cwd[FN_REFLEN] = "";
		getcwd(cwd, sizeof(cwd));
		ib::error() << "current dir = " << cwd;
		ib::error() << "srv_data_home = " << srv_data_home;
		ib::error() << "Log marker path: " << log_marker_path;
		perror("");
		os_file_get_last_error(true);
		return(DB_ERROR);
	}

	ib::info() << "Created log markers file at "
		   << log_marker_path;
	return(DB_SUCCESS);
}

static
dberr_t
log_markers_open(bool &markers_exist)
{
	bool success;

	log_markers_make_path();

	log_marker_file
		= os_file_create(PSI_NOT_INSTRUMENTED, // TODO
				 log_marker_path,
				 OS_FILE_OPEN | OS_FILE_ON_ERROR_NO_EXIT
				 | OS_FILE_ON_ERROR_SILENT,
				 OS_FILE_NORMAL,
				 OS_DATA_TEMP_FILE,
				 true, &success);

	if (!success) {
		markers_exist = false;
		if (errno == ENOENT)
			return(DB_SUCCESS);

		char cwd[FN_REFLEN] = "";
		getcwd(cwd, sizeof(cwd));
		ib::error() << "current dir = " << cwd;
		ib::error() << "srv_data_home = " << srv_data_home;
		ib::error() << "Log marker path: " << log_marker_path;
		perror("");
		os_file_get_last_error(true);
		return(DB_ERROR);
	}

	ib::info() << "Opened log markers file at "
		   << log_marker_path;
	markers_exist = true;
	return(DB_SUCCESS);

}

static
void
log_markers_close_file(void)
{
	os_file_close(log_marker_file);
	log_marker_file = OS_FILE_CLOSED;
	os_file_delete_if_exists(PSI_NOT_INSTRUMENTED, // TODO
				 log_marker_path, NULL);
	ib::info() << "Closed and deleted log markers file at "
		   << log_marker_path;
}

void
log_markers_close(void)
{
	log_markers_close_file();
	mutex_free(&log_marker_sys_mutex);
}

void
log_markers_maybe_mark(lsn_t lsn)
{
	ut_ad(log_marker_file != OS_FILE_CLOSED);
	ut_ad(!srv_read_only_mode);

	if (lsn - last_marked_lsn < LSN_MARKING_STEP)
		return;

	mutex_enter(&log_marker_sys_mutex);

	if (lsn - last_marked_lsn < LSN_MARKING_STEP) {
		mutex_exit(&log_marker_sys_mutex);
		return;
	}

	last_marked_lsn = lsn;

	IORequest write_request(IORequest::WRITE | IORequest::NO_COMPRESSION);

	os_file_write(write_request, log_marker_path, log_marker_file, &lsn,
		      log_marker_file_offset, sizeof(lsn));
	log_marker_file_offset += sizeof(lsn);

	mutex_exit(&log_marker_sys_mutex);

	bool success = os_file_flush(log_marker_file);
	ut_ad(success);
}

std::vector<lsn_range_t>
log_markers_get(lsn_t checkpoint_lsn, lsn_t log_end_lsn, dberr_t &err)
{
	bool markers_exist;

	err = log_markers_open(markers_exist);
	if (err != DB_SUCCESS)
		return(std::vector<lsn_range_t>());

	if (!markers_exist) {
		ib::info()
			<< "No redo log markers, falling back to "
			"single-threaded recovery";
		return(std::vector<lsn_range_t >(1,
			   std::make_pair<lsn_t, lsn_t>(checkpoint_lsn,
							log_end_lsn)));
	}

	os_offset_t marker_file_size = os_file_get_size(log_marker_file);

	ib::info() << "Partitioning LSN recovery range from " << checkpoint_lsn
		   << " to " << log_end_lsn << " into "
		   << LSN_MARKER_PARTITION_COUNT
		   << " partitions, using marker file of size "
		   << marker_file_size << " bytes";

	os_offset_t read_offset = 0;

	IORequest read_request(IORequest::READ | IORequest::NO_COMPRESSION);
	lsn_t lsn = 0;

	while (read_offset + sizeof(lsn) <= marker_file_size
	       && lsn < checkpoint_lsn) {
		err = os_file_read(read_request, log_marker_file, &lsn,
				   read_offset, sizeof(lsn));
		if (err != DB_SUCCESS) {
			os_file_close(log_marker_file);
			log_marker_file = OS_FILE_CLOSED;
			return(std::vector<lsn_range_t >());
		}
		read_offset += sizeof(lsn);
	}

	ib::info() << "First LSN at or later than checkpoint is " << lsn
		   << " at " << read_offset - sizeof(lsn);

	ulint available_markers = (marker_file_size - read_offset)
		/ sizeof(lsn);
	ulint partition_count = LSN_MARKER_PARTITION_COUNT;
	if (available_markers < partition_count - 1) {

		ib::info() << "Fewer markers (" << available_markers
			   << ") found than needed to partition the LSN "
			"range to " << partition_count << " partitions";
		partition_count = available_markers + 1;
		ut_ad(partition_count < LSN_MARKER_PARTITION_COUNT);
		ut_ad(read_offset + (partition_count - 1) * sizeof(lsn)
		      == marker_file_size);
	}
	ib::info() << "Partitioning LSN range into " << partition_count
		   << " partitions ";

	std::vector<lsn_range_t > result(partition_count);
	result[0].first = checkpoint_lsn;
	result[partition_count - 1].second = log_end_lsn;

	if (partition_count == 1) {
		log_markers_close_file();
		return(result);
	}

	read_offset -= sizeof(lsn);
	for (ulint i = 1; i < partition_count; i++) {

		read_offset += available_markers / (partition_count - 1)
			* sizeof(lsn);
		err = os_file_read(read_request, log_marker_file,
				   &result[i].first, read_offset, sizeof(lsn));
		result[i - 1].second = result[i].first;
		if (err != DB_SUCCESS) {
			ib::error() << "Log marker read error at offset "
				    << read_offset;
			os_file_close(log_marker_file);
			log_marker_file = OS_FILE_CLOSED;
			return(std::vector<lsn_range_t >());
		}
	}
	log_markers_close_file();

	for (ulint i = 0; i < partition_count; i++) {
		ib::info() << "Partition " << result[i].first << " to "
			   << result[i].second;
		ut_ad(result[i].first < result[i].second);
		ut_ad(i > 0 ? result[i].first == result[i - 1].second
		      : result[i].first == checkpoint_lsn);
	}

	return(result);
}
