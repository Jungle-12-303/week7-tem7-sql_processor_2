#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Grows the record pointer array when it becomes full. */
static int table_ensure_capacity(Table *table) {
    size_t new_capacity;
    Record **new_rows;

    if (table->size < table->capacity) {
        return 1;
    }

    new_capacity = (table->capacity == 0) ? 8 : table->capacity * 2;
    new_rows = (Record **)realloc(table->rows, new_capacity * sizeof(Record *));

    if (new_rows == NULL) {
        return 0;
    }

    table->rows = new_rows;
    table->capacity = new_capacity;
    return 1;
}

/* Creates the single in-memory users table. */
Table *table_create(void) {
    Table *table = (Table *)calloc(1, sizeof(Table));

    if (table == NULL) {
        return NULL;
    }

    table->next_id = 1;
    table->pk_index = bptree_create();

    if (table->pk_index == NULL) {
        free(table);
        return NULL;
    }

    return table;
}

/* Frees all table-owned records and the B+ tree index. */
void table_destroy(Table *table) {
    size_t index;

    if (table == NULL) {
        return;
    }

    for (index = 0; index < table->size; index++) {
        free(table->rows[index]);
    }

    free(table->rows);
    bptree_destroy(table->pk_index);
    free(table);
}

/* Inserts one record and returns the stored record pointer. */
Record *table_insert(Table *table, const char *name, int age) {
    Record *record;

    if (table == NULL || name == NULL) {
        return NULL;
    }

    if (!table_ensure_capacity(table)) {
        return NULL;
    }

    record = (Record *)calloc(1, sizeof(Record));

    if (record == NULL) {
        return NULL;
    }

    record->id = table->next_id++;
    strncpy(record->name, name, RECORD_NAME_SIZE - 1);
    record->name[RECORD_NAME_SIZE - 1] = '\0';
    record->age = age;

    table->rows[table->size] = record;

    if (!bptree_insert(table->pk_index, record->id, record)) {
        free(record);
        return NULL;
    }

    table->size++;
    return record;
}

/* Looks up a record by ID using the B+ tree index. */
Record *table_find_by_id(Table *table, int id) {
    if (table == NULL) {
        return NULL;
    }

    return (Record *)bptree_search(table->pk_index, id);
}

/* Looks up a record by ID using a linear scan for benchmarking. */
Record *table_scan_by_id(Table *table, int id) {
    size_t index;

    if (table == NULL) {
        return NULL;
    }

    for (index = 0; index < table->size; index++) {
        if (table->rows[index]->id == id) {
            return table->rows[index];
        }
    }

    return NULL;
}

/* Looks up the first record with a matching name using a linear scan. */
Record *table_find_by_name(Table *table, const char *name) {
    size_t index;

    if (table == NULL || name == NULL) {
        return NULL;
    }

    for (index = 0; index < table->size; index++) {
        if (strcmp(table->rows[index]->name, name) == 0) {
            return table->rows[index];
        }
    }

    return NULL;
}

/* Looks up the first record with a matching age using a linear scan. */
Record *table_find_by_age(Table *table, int age) {
    size_t index;

    if (table == NULL) {
        return NULL;
    }

    for (index = 0; index < table->size; index++) {
        if (table->rows[index]->age == age) {
            return table->rows[index];
        }
    }

    return NULL;
}

/* Prints a single record in a compact presentation-friendly format. */
void table_print_record(const Record *record) {
    if (record == NULL) {
        return;
    }

    printf("id=%d, name='%s', age=%d\n", record->id, record->name, record->age);
}
