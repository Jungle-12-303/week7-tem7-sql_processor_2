#ifndef SQL_H
#define SQL_H

#include "table.h"
#include <stddef.h>

typedef enum SQLStatus {
    SQL_STATUS_OK,
    SQL_STATUS_NOT_FOUND,
    SQL_STATUS_SYNTAX_ERROR,
    SQL_STATUS_EXIT,
    SQL_STATUS_ERROR
} SQLStatus;

typedef enum SQLAction {
    SQL_ACTION_NONE,
    SQL_ACTION_INSERT,
    SQL_ACTION_SELECT_ROWS
} SQLAction;

typedef struct SQLResult {
    SQLStatus status;
    SQLAction action;
    Record *record;
    Record **records;
    int inserted_id;
    size_t row_count;
} SQLResult;

/* Parses one SQL statement and executes it against the table. */
SQLResult sql_execute(Table *table, const char *input);

/* Releases any heap memory owned by a SQL result. */
void sql_result_destroy(SQLResult *result);

#endif
