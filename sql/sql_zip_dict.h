/*****************************************************************************

Copyright (c) 2010-2016, Percona Inc. All Rights Reserved.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA

*****************************************************************************/

#ifndef SQL_ZIP_DICT_INCLUDED
#define SQL_ZIP_DICT_INCLUDED

#include "my_global.h"

class THD;

  /**
    Creates a new compression dictionary with the specified data.

    @param thd                        thread descriptor.
    @param name                       compression dictionary name
    @param name_len                   compression dictionary name length
    @param data                       compression dictionary data
    @param data_len                   compression dictionary data length

    @return HA_CREATE_ZIP_DICT_OK             - on success
            HA_CREATE_ZIP_DICT_ALREADY_EXISTS - if dictionary with such name
                                                already exists
            HA_CREATE_ZIP_DICT_NAME_TOO_LONG  - if dictionary name is too long
            HA_CREATE_ZIP_DICT_DATA_TOO_LONG  - if dictionary data is too long
            HA_CREATE_ZIP_DICT_UNKNOWN_ERROR  - if unexpected error occurred
            HA_ADMIN_NOT_IMPLEMENTED          - if current engine does not
                                                support compression
                                                dictionaries
  */
int mysql_create_zip_dict(THD* thd, const char* name, ulong name_len,
  const char* data, ulong data_len);

  /**
    Deletes a compression dictionary.

    @param thd                        thread descriptor.
    @param name                       compression dictionary name
    @param name_len                   compression dictionary name length

    @return HA_DROP_ZIP_DICT_OK,            - on success
            HA_DROP_ZIP_DICT_DOES_NOT_EXIST - if dictionary with such name does
                                              not exist
            HA_DROP_ZIP_DICT_IS_REFERENCED, - if dictictionary is still in use
            HA_DROP_ZIP_DICT_UNKNOWN_ERROR  - if unexpected error occurred
            HA_ADMIN_NOT_IMPLEMENTED          - if current engine does not
                                                support compression
                                                dictionaries
  */
int mysql_drop_zip_dict(THD* thd, const char* name, ulong name_len);

#endif /* SQL_ZIP_DICT_INCLUDED */
