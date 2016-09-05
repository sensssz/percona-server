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

/* create and drop zip_dicts */

#include "sql_zip_dict.h"
#include "sql_table.h"                          // write_bin_log
#include "sql_class.h"                          // THD

int mysql_create_zip_dict(THD* thd, const char* name, ulong name_len, const char* data, ulong data_len)
{
  int error= HA_ADMIN_NOT_IMPLEMENTED;

  DBUG_ENTER("mysql_create_zip_dict");
  handlerton *hton= ha_default_handlerton(thd);

  if(!hton->create_zip_dict)
  {
    my_error(ER_ILLEGAL_HA_CREATE_OPTION, MYF(0),
      ha_resolve_storage_engine_name(hton), "COMPRESSED COLUMNS");
    DBUG_RETURN(error);
  }

  ulong local_name_len = name_len;
  ulong local_data_len = data_len;
  handler_create_zip_dict_result create_result =
    hton->create_zip_dict(hton, thd, name, &local_name_len, data, &local_data_len);
  if(create_result != HA_CREATE_ZIP_DICT_OK)
  {
    switch(create_result)
    {
      case HA_CREATE_ZIP_DICT_NAME_TOO_LONG:
        error = ER_COMPRESSION_DICTIONARY_NAME_TOO_LONG;
        my_error(error, MYF(0), name, local_name_len);
        break;
      case HA_CREATE_ZIP_DICT_DATA_TOO_LONG:
        error = ER_COMPRESSION_DICTIONARY_DATA_TOO_LONG;
        my_error(error, MYF(0), name, local_data_len);
        break;
      case HA_CREATE_ZIP_DICT_ALREADY_EXISTS:
        error = ER_COMPRESSION_DICTIONARY_EXISTS;
        my_error(error, MYF(0), name);
        break;
      case HA_CREATE_ZIP_DICT_READ_ONLY:
        error = ER_READ_ONLY_MODE;
        my_error(error, MYF(0));
        break;
      default:
        DBUG_ASSERT(0);
        error = ER_UNKNOWN_ERROR;
        my_error(error, MYF(0));
    }
    DBUG_RETURN(error);
  }

  error = write_bin_log(thd, FALSE, thd->query(), thd->query_length());
  DBUG_RETURN(error);
}

int mysql_drop_zip_dict(THD* thd, const char* name, ulong name_len)
{
  int error= HA_ADMIN_NOT_IMPLEMENTED;

  DBUG_ENTER("mysql_drop_zip_dict");
  handlerton *hton= ha_default_handlerton(thd);

  if(!hton->drop_zip_dict)
  {
    my_error(ER_ILLEGAL_HA_CREATE_OPTION, MYF(0),
      ha_resolve_storage_engine_name(hton), "COMPRESSED COLUMNS");
    DBUG_RETURN(error);
  }

  ulong local_name_len = name_len;
  handler_drop_zip_dict_result drop_result =
    hton->drop_zip_dict(hton, thd, name, &local_name_len);
  if(drop_result != HA_DROP_ZIP_DICT_OK)
  {
    switch(drop_result)
    {
      case HA_DROP_ZIP_DICT_DOES_NOT_EXIST:
        error = ER_COMPRESSION_DICTIONARY_DOES_NOT_EXIST;
        my_error(error, MYF(0), name);
        break;
      case HA_DROP_ZIP_DICT_IS_REFERENCED:
        error = ER_COMPRESSION_DICTIONARY_IS_REFERENCED;
        my_error(error, MYF(0), name);
        break;
      case HA_DROP_ZIP_DICT_READ_ONLY:
        error = ER_READ_ONLY_MODE;
        my_error(error, MYF(0));
        break;
      default:
        DBUG_ASSERT(0);
        error = ER_UNKNOWN_ERROR;
        my_error(error, MYF(0));
    }
    DBUG_RETURN(error);
  }

  error = write_bin_log(thd, FALSE, thd->query(), thd->query_length());
  DBUG_RETURN(error);
}
