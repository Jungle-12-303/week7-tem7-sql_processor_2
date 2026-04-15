#ifndef SQL_H
#define SQL_H

#include "table.h"

typedef enum SQLStatus {
    SQL_STATUS_OK,
    SQL_STATUS_NOT_FOUND,
    SQL_STATUS_SYNTAX_ERROR,
    SQL_STATUS_EXIT,
    SQL_STATUS_ERROR
} SQLStatus;

typedef struct SQLResult {
    SQLStatus status;
    Record *record;
    int inserted_id;
} SQLResult;

/* Parses one SQL statement and executes it against the table. */
SQLResult sql_execute(Table *table, const char *input);

#endif
