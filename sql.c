#include "sql.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>

/* Skips whitespace characters in the parser cursor. */
static void sql_skip_spaces(const char **cursor) {
    while (**cursor != '\0' && isspace((unsigned char)**cursor)) {
        (*cursor)++;
    }
}

/* Matches a keyword case-insensitively and requires a token boundary. */
static int sql_match_keyword(const char **cursor, const char *keyword) {
    const char *start = *cursor;
    const char *word = keyword;

    while (*word != '\0') {
        if (tolower((unsigned char)*start) != tolower((unsigned char)*word)) {
            return 0;
        }
        start++;
        word++;
    }

    if (isalnum((unsigned char)*start) || *start == '_') {
        return 0;
    }

    *cursor = start;
    return 1;
}

/* Matches a single expected punctuation character. */
static int sql_match_char(const char **cursor, char expected) {
    sql_skip_spaces(cursor);

    if (**cursor != expected) {
        return 0;
    }

    (*cursor)++;
    return 1;
}

/* Parses an identifier such as users, id, name, or age. */
static int sql_parse_identifier(const char **cursor, char *buffer, size_t buffer_size) {
    size_t length = 0;

    sql_skip_spaces(cursor);

    if (!(isalpha((unsigned char)**cursor) || **cursor == '_')) {
        return 0;
    }

    while ((isalnum((unsigned char)**cursor) || **cursor == '_') && length + 1 < buffer_size) {
        buffer[length++] = **cursor;
        (*cursor)++;
    }

    if (length == 0) {
        return 0;
    }

    while (isalnum((unsigned char)**cursor) || **cursor == '_') {
        (*cursor)++;
        length++;
    }

    if (length >= buffer_size) {
        return 0;
    }

    buffer[length] = '\0';
    return 1;
}

/* Parses a single-quoted string literal. */
static int sql_parse_string(const char **cursor, char *buffer, size_t buffer_size) {
    size_t length = 0;

    sql_skip_spaces(cursor);

    if (**cursor != '\'') {
        return 0;
    }

    (*cursor)++;

    while (**cursor != '\0' && **cursor != '\'') {
        if (length + 1 >= buffer_size) {
            return 0;
        }
        buffer[length++] = **cursor;
        (*cursor)++;
    }

    if (**cursor != '\'') {
        return 0;
    }

    (*cursor)++;
    buffer[length] = '\0';
    return 1;
}

/* Parses a signed integer literal. */
static int sql_parse_int(const char **cursor, int *value) {
    char *end_ptr;
    long parsed;

    sql_skip_spaces(cursor);

    if (!(isdigit((unsigned char)**cursor) || **cursor == '-')) {
        return 0;
    }

    parsed = strtol(*cursor, &end_ptr, 10);

    if (end_ptr == *cursor) {
        return 0;
    }

    *value = (int)parsed;
    *cursor = end_ptr;
    return 1;
}

/* Accepts an optional semicolon and trailing whitespace at the end. */
static int sql_match_statement_end(const char **cursor) {
    sql_skip_spaces(cursor);

    if (**cursor == ';') {
        (*cursor)++;
    }

    sql_skip_spaces(cursor);
    return **cursor == '\0';
}

/* Parses and executes the fixed INSERT syntax. */
static SQLResult sql_execute_insert(Table *table, const char *input) {
    SQLResult result = {SQL_STATUS_SYNTAX_ERROR, NULL, 0};
    const char *cursor = input;
    char table_name[32];
    char name[RECORD_NAME_SIZE];
    int age;
    Record *record;

    if (!sql_match_keyword(&cursor, "INSERT")) {
        return result;
    }

    sql_skip_spaces(&cursor);
    if (!sql_match_keyword(&cursor, "INTO")) {
        return result;
    }

    if (!sql_parse_identifier(&cursor, table_name, sizeof(table_name))) {
        return result;
    }

    if (strcasecmp(table_name, "users") != 0) {
        return result;
    }

    sql_skip_spaces(&cursor);
    if (!sql_match_keyword(&cursor, "VALUES")) {
        return result;
    }

    if (!sql_match_char(&cursor, '(')) {
        return result;
    }

    if (!sql_parse_string(&cursor, name, sizeof(name))) {
        return result;
    }

    if (!sql_match_char(&cursor, ',')) {
        return result;
    }

    if (!sql_parse_int(&cursor, &age)) {
        return result;
    }

    if (!sql_match_char(&cursor, ')')) {
        return result;
    }

    if (!sql_match_statement_end(&cursor)) {
        return result;
    }

    record = table_insert(table, name, age);

    if (record == NULL) {
        result.status = SQL_STATUS_ERROR;
        return result;
    }

    result.status = SQL_STATUS_OK;
    result.record = record;
    result.inserted_id = record->id;
    return result;
}

/* Parses and executes the fixed SELECT syntax. */
static SQLResult sql_execute_select(Table *table, const char *input) {
    SQLResult result = {SQL_STATUS_SYNTAX_ERROR, NULL, 0};
    const char *cursor = input;
    char table_name[32];
    char column[32];
    char text_value[RECORD_NAME_SIZE];
    int int_value;

    if (!sql_match_keyword(&cursor, "SELECT")) {
        return result;
    }

    if (!sql_match_char(&cursor, '*')) {
        return result;
    }

    sql_skip_spaces(&cursor);
    if (!sql_match_keyword(&cursor, "FROM")) {
        return result;
    }

    if (!sql_parse_identifier(&cursor, table_name, sizeof(table_name))) {
        return result;
    }

    if (strcasecmp(table_name, "users") != 0) {
        return result;
    }

    sql_skip_spaces(&cursor);
    if (!sql_match_keyword(&cursor, "WHERE")) {
        return result;
    }

    if (!sql_parse_identifier(&cursor, column, sizeof(column))) {
        return result;
    }

    if (!sql_match_char(&cursor, '=')) {
        return result;
    }

    if (strcasecmp(column, "id") == 0) {
        if (!sql_parse_int(&cursor, &int_value) || !sql_match_statement_end(&cursor)) {
            return result;
        }

        result.record = table_find_by_id(table, int_value);
    } else if (strcasecmp(column, "name") == 0) {
        if (!sql_parse_string(&cursor, text_value, sizeof(text_value)) || !sql_match_statement_end(&cursor)) {
            return result;
        }

        result.record = table_find_by_name(table, text_value);
    } else if (strcasecmp(column, "age") == 0) {
        if (!sql_parse_int(&cursor, &int_value) || !sql_match_statement_end(&cursor)) {
            return result;
        }

        result.record = table_find_by_age(table, int_value);
    } else {
        return result;
    }

    result.status = (result.record == NULL) ? SQL_STATUS_NOT_FOUND : SQL_STATUS_OK;
    return result;
}

/* Parses one SQL statement and executes it against the table. */
SQLResult sql_execute(Table *table, const char *input) {
    SQLResult result = {SQL_STATUS_SYNTAX_ERROR, NULL, 0};
    const char *cursor;

    if (table == NULL || input == NULL) {
        result.status = SQL_STATUS_ERROR;
        return result;
    }

    cursor = input;
    sql_skip_spaces(&cursor);

    if (sql_match_keyword(&cursor, "EXIT") || sql_match_keyword(&cursor, "QUIT")) {
        result.status = sql_match_statement_end(&cursor) ? SQL_STATUS_EXIT : SQL_STATUS_SYNTAX_ERROR;
        return result;
    }

    result = sql_execute_insert(table, input);
    if (result.status != SQL_STATUS_SYNTAX_ERROR) {
        return result;
    }

    result = sql_execute_select(table, input);
    return result;
}
