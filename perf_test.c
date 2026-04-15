#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define PERF_INSERT_COUNT 1000000
#define PERF_SEARCH_COUNT 10000

/* Returns the current wall-clock time in milliseconds. */
static double now_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

/* Builds predictable sample IDs so every run measures the same lookups. */
static void build_sample_ids(int *sample_ids, int count, int max_id) {
    int index;

    for (index = 0; index < count; index++) {
        sample_ids[index] = ((index * 97) % max_id) + 1;
    }
}

/* Inserts one million rows and compares indexed ID search with linear scan. */
int main(void) {
    Table *table = table_create();
    int *sample_ids;
    int index;
    double start_ms;
    double bptree_ms;
    double linear_ms;
    double speedup;
    long long checksum_index = 0;
    long long checksum_linear = 0;

    if (table == NULL) {
        fprintf(stderr, "Failed to create table.\n");
        return 1;
    }

    sample_ids = (int *)malloc(sizeof(int) * PERF_SEARCH_COUNT);
    if (sample_ids == NULL) {
        fprintf(stderr, "Failed to allocate sample IDs.\n");
        table_destroy(table);
        return 1;
    }

    for (index = 0; index < PERF_INSERT_COUNT; index++) {
        char name[RECORD_NAME_SIZE];

        snprintf(name, sizeof(name), "user%d", index + 1);
        if (table_insert(table, name, index % 100) == NULL) {
            fprintf(stderr, "Insert failed at row %d.\n", index + 1);
            free(sample_ids);
            table_destroy(table);
            return 1;
        }
    }

    build_sample_ids(sample_ids, PERF_SEARCH_COUNT, PERF_INSERT_COUNT);

    start_ms = now_ms();
    for (index = 0; index < PERF_SEARCH_COUNT; index++) {
        Record *record = table_find_by_id(table, sample_ids[index]);
        if (record != NULL) {
            checksum_index += record->id;
        }
    }
    bptree_ms = now_ms() - start_ms;

    start_ms = now_ms();
    for (index = 0; index < PERF_SEARCH_COUNT; index++) {
        Record *record = table_scan_by_id(table, sample_ids[index]);
        if (record != NULL) {
            checksum_linear += record->id;
        }
    }
    linear_ms = now_ms() - start_ms;

    if (checksum_index != checksum_linear) {
        fprintf(stderr, "Search results mismatch.\n");
        free(sample_ids);
        table_destroy(table);
        return 1;
    }

    speedup = (bptree_ms > 0.0) ? (linear_ms / bptree_ms) : 0.0;

    printf("+---------------+---------------------------+--------------------------+---------+\n");
    printf("| Inserted Rows | B+ Tree Search Time (ms) | Linear Search Time (ms) | Speedup |\n");
    printf("+---------------+---------------------------+--------------------------+---------+\n");
    printf("| %13d | %25.3f | %24.3f | %7.2fx |\n", PERF_INSERT_COUNT, bptree_ms, linear_ms, speedup);
    printf("+---------------+---------------------------+--------------------------+---------+\n");

    free(sample_ids);
    table_destroy(table);
    return 0;
}
