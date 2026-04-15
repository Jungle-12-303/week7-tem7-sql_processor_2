#include "sql.h"
#include "table.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define PERF_INSERT_COUNT 1000000
#define EXACT_QUERY_COUNT 1000
#define RANGE_QUERY_COUNT 100

/* Returns the current wall-clock time in milliseconds. */
static double now_ms(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000.0 + (double)tv.tv_usec / 1000.0;
}

/* Builds deterministic exact-match samples for repeatable timing. */
static void build_exact_samples(int *sample_ids, int *sample_ages, int count, int max_id) {
    int index;

    for (index = 0; index < count; index++) {
        sample_ids[index] = ((index * 97) % max_id) + 1;
        sample_ages[index] = sample_ids[index] % 100;
    }
}

/* Runs parameterized WHERE queries and returns the elapsed time. */
static double benchmark_parameterized_queries(
    Table *table,
    const char *query_prefix,
    const int *values,
    int query_count,
    long long *row_checksum,
    long long *first_id_checksum
) {
    char query[128];
    int index;
    double start_ms = now_ms();

    *row_checksum = 0;
    *first_id_checksum = 0;

    for (index = 0; index < query_count; index++) {
        SQLResult result;

        snprintf(query, sizeof(query), "%s %d;", query_prefix, values[index]);
        result = sql_execute(table, query);

        if (result.status != SQL_STATUS_OK) {
            fprintf(stderr, "Query failed: %s\n", query);
            sql_result_destroy(&result);
            return -1.0;
        }

        *row_checksum += (long long)result.row_count;
        if (result.record != NULL) {
            *first_id_checksum += result.record->id;
        }

        sql_result_destroy(&result);
    }

    return now_ms() - start_ms;
}

/* Runs the same WHERE query repeatedly and returns the elapsed time. */
static double benchmark_repeated_query(
    Table *table,
    const char *query,
    int repeat_count,
    long long *row_checksum,
    long long *first_id_checksum
) {
    int index;
    double start_ms = now_ms();

    *row_checksum = 0;
    *first_id_checksum = 0;

    for (index = 0; index < repeat_count; index++) {
        SQLResult result = sql_execute(table, query);

        if (result.status != SQL_STATUS_OK) {
            fprintf(stderr, "Query failed: %s\n", query);
            sql_result_destroy(&result);
            return -1.0;
        }

        *row_checksum += (long long)result.row_count;
        if (result.record != NULL) {
            *first_id_checksum += result.record->id;
        }

        sql_result_destroy(&result);
    }

    return now_ms() - start_ms;
}

/* Inserts sample data, then compares id-based and age-based WHERE performance. */
int main(void) {
    Table *table = table_create();
    int *sample_ids;
    int *sample_ages;
    int index;
    long long id_exact_rows;
    long long age_exact_rows;
    long long id_exact_first_ids;
    long long age_exact_first_ids;
    long long id_range_rows;
    long long age_range_rows;
    long long id_range_first_ids;
    long long age_range_first_ids;
    double id_exact_ms;
    double age_exact_ms;
    double id_range_ms;
    double age_range_ms;

    if (table == NULL) {
        fprintf(stderr, "Failed to create table.\n");
        return 1;
    }

    sample_ids = (int *)malloc(sizeof(int) * EXACT_QUERY_COUNT);
    sample_ages = (int *)malloc(sizeof(int) * EXACT_QUERY_COUNT);
    if (sample_ids == NULL || sample_ages == NULL) {
        fprintf(stderr, "Failed to allocate benchmark samples.\n");
        free(sample_ids);
        free(sample_ages);
        table_destroy(table);
        return 1;
    }

    for (index = 0; index < PERF_INSERT_COUNT; index++) {
        char name[RECORD_NAME_SIZE];

        snprintf(name, sizeof(name), "user%d", index + 1);
        if (table_insert(table, name, index % 100) == NULL) {
            fprintf(stderr, "Insert failed at row %d.\n", index + 1);
            free(sample_ids);
            free(sample_ages);
            table_destroy(table);
            return 1;
        }
    }

    build_exact_samples(sample_ids, sample_ages, EXACT_QUERY_COUNT, PERF_INSERT_COUNT);

    id_exact_ms = benchmark_parameterized_queries(
        table,
        "SELECT * FROM users WHERE id =",
        sample_ids,
        EXACT_QUERY_COUNT,
        &id_exact_rows,
        &id_exact_first_ids
    );
    age_exact_ms = benchmark_parameterized_queries(
        table,
        "SELECT * FROM users WHERE age =",
        sample_ages,
        EXACT_QUERY_COUNT,
        &age_exact_rows,
        &age_exact_first_ids
    );
    id_range_ms = benchmark_repeated_query(
        table,
        "SELECT * FROM users WHERE id >= 990001;",
        RANGE_QUERY_COUNT,
        &id_range_rows,
        &id_range_first_ids
    );
    age_range_ms = benchmark_repeated_query(
        table,
        "SELECT * FROM users WHERE age >= 99;",
        RANGE_QUERY_COUNT,
        &age_range_rows,
        &age_range_first_ids
    );

    if (id_exact_ms < 0.0 || age_exact_ms < 0.0 || id_range_ms < 0.0 || age_range_ms < 0.0) {
        free(sample_ids);
        free(sample_ages);
        table_destroy(table);
        return 1;
    }

    printf("Exact-match WHERE benchmark (%d queries)\n", EXACT_QUERY_COUNT);
    printf("+-------------------------------+-----------+------------------+----------------+\n");
    printf("| Query                         | Avg. Rows | Time (ms)        | First-ID Check |\n");
    printf("+-------------------------------+-----------+------------------+----------------+\n");
    printf("| SELECT * WHERE id = ?         | %9.2f | %16.3f | %14lld |\n",
           (double)id_exact_rows / (double)EXACT_QUERY_COUNT, id_exact_ms, id_exact_first_ids);
    printf("| SELECT * WHERE age = ?        | %9.2f | %16.3f | %14lld |\n",
           (double)age_exact_rows / (double)EXACT_QUERY_COUNT, age_exact_ms, age_exact_first_ids);
    printf("+-------------------------------+-----------+------------------+----------------+\n");
    printf("\n");

    printf("Range WHERE benchmark (%d repeated queries, 10,000 rows/query)\n", RANGE_QUERY_COUNT);
    printf("+-------------------------------+-----------+------------------+----------------+\n");
    printf("| Query                         | Avg. Rows | Time (ms)        | First-ID Check |\n");
    printf("+-------------------------------+-----------+------------------+----------------+\n");
    printf("| SELECT * WHERE id >= 990001   | %9.2f | %16.3f | %14lld |\n",
           (double)id_range_rows / (double)RANGE_QUERY_COUNT, id_range_ms, id_range_first_ids);
    printf("| SELECT * WHERE age >= 99      | %9.2f | %16.3f | %14lld |\n",
           (double)age_range_rows / (double)RANGE_QUERY_COUNT, age_range_ms, age_range_first_ids);
    printf("+-------------------------------+-----------+------------------+----------------+\n");

    free(sample_ids);
    free(sample_ages);
    table_destroy(table);
    return 0;
}
