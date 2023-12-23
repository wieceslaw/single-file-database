//
// Created by wieceslaw on 23.12.23.
//

#ifndef SINGLE_FILE_DATABASE_TABLESMAP_H
#define SINGLE_FILE_DATABASE_TABLESMAP_H

#include "map/map.h"
#include "map/map_impl.h"
#include "database/table/table.h"

DECLARE_MAP(char*, table_scheme*, StrTableSchemeMap)
#define MAP_NEW_STR_TABLE_SCHEME(C) MAP_NEW(StrTableSchemeMap, C, str_hash, str_equals, str_copy, str_free, (copy_f) table_scheme_copy, (free_f) table_scheme_free)

#endif //SINGLE_FILE_DATABASE_TABLESMAP_H
