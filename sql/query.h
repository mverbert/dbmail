#ifndef _QUERY_H
#define _QUERY_H 1

extern const char _binary_create_tables_sqlite_start;

#define DM_QUERY(ext, from_version, to_version) &_binary_ ## to_version ## _ ## ext ## _start
#define DECLARE_EXTERN_QUERY(ext, from_version, to_version) extern const char _binary_ ## to_version ## _ ## ext ## _start

DECLARE_EXTERN_QUERY(sqlite, 0, 32001);
DECLARE_EXTERN_QUERY(sqlite, 32001, 32002);
DECLARE_EXTERN_QUERY(sqlite, 32001, 32003);
DECLARE_EXTERN_QUERY(sqlite, 32001, 32004);

DECLARE_EXTERN_QUERY(mysql, 0, 32001);
DECLARE_EXTERN_QUERY(mysql, 32001, 32002);
DECLARE_EXTERN_QUERY(mysql, 32001, 32003);
DECLARE_EXTERN_QUERY(mysql, 32001, 32004);

DECLARE_EXTERN_QUERY(psql, 0, 32001);
DECLARE_EXTERN_QUERY(psql, 32001, 32002);
DECLARE_EXTERN_QUERY(psql, 32001, 32003);
DECLARE_EXTERN_QUERY(psql, 32001, 32004);

#endif
