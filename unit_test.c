#include "bptree.h"
#include "sql.h"
#include "table.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Verifies searching an empty tree returns NULL. */
static void test_empty_tree_search(void) {
    BPTree *tree = bptree_create();

    assert(tree != NULL);
    assert(bptree_search(tree, 10) == NULL);

    bptree_destroy(tree);
}

/* Verifies a single inserted key can be found again. */
static void test_single_insert_search(void) {
    BPTree *tree = bptree_create();
    int value = 123;

    assert(tree != NULL);
    assert(bptree_insert(tree, 1, &value) == 1);
    assert(*(int *)bptree_search(tree, 1) == 123);

    bptree_destroy(tree);
}

/* Verifies duplicate keys are rejected. */
static void test_duplicate_key_rejected(void) {
    BPTree *tree = bptree_create();
    int value1 = 100;
    int value2 = 200;

    assert(tree != NULL);
    assert(bptree_insert(tree, 7, &value1) == 1);
    assert(bptree_insert(tree, 7, &value2) == 0);
    assert(*(int *)bptree_search(tree, 7) == 100);

    bptree_destroy(tree);
}

/* Verifies a leaf split still preserves every inserted key. */
static void test_leaf_split_search(void) {
    BPTree *tree = bptree_create();
    int values[4] = {10, 20, 30, 40};
    int index;

    assert(tree != NULL);

    for (index = 0; index < 4; index++) {
        assert(bptree_insert(tree, index + 1, &values[index]) == 1);
    }

    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 0);
    assert(tree->root->num_keys == 1);

    for (index = 0; index < 4; index++) {
        assert(*(int *)bptree_search(tree, index + 1) == values[index]);
    }

    bptree_destroy(tree);
}

/* Verifies internal splits can grow the tree height above one level. */
static void test_internal_split_new_root(void) {
    BPTree *tree = bptree_create();
    int values[10];
    int index;

    assert(tree != NULL);

    for (index = 0; index < 10; index++) {
        values[index] = index + 1;
        assert(bptree_insert(tree, index + 1, &values[index]) == 1);
    }

    assert(tree->root != NULL);
    assert(tree->root->is_leaf == 0);
    assert(tree->root->children[0] != NULL);
    assert(tree->root->children[0]->is_leaf == 0);

    for (index = 0; index < 10; index++) {
        assert(*(int *)bptree_search(tree, index + 1) == values[index]);
    }

    bptree_destroy(tree);
}

/* Verifies leaf siblings remain linked after a split. */
static void test_leaf_next_link(void) {
    BPTree *tree = bptree_create();
    int values[4] = {1, 2, 3, 4};
    BPTreeNode *left_leaf;
    BPTreeNode *right_leaf;

    assert(tree != NULL);
    assert(bptree_insert(tree, 1, &values[0]) == 1);
    assert(bptree_insert(tree, 2, &values[1]) == 1);
    assert(bptree_insert(tree, 3, &values[2]) == 1);
    assert(bptree_insert(tree, 4, &values[3]) == 1);

    left_leaf = tree->root->children[0];
    right_leaf = tree->root->children[1];

    assert(left_leaf != NULL);
    assert(right_leaf != NULL);
    assert(left_leaf->is_leaf == 1);
    assert(left_leaf->next == right_leaf);

    bptree_destroy(tree);
}

/* Verifies the table assigns monotonically increasing IDs. */
static void test_table_auto_increment(void) {
    Table *table = table_create();
    Record *first;
    Record *second;

    assert(table != NULL);

    first = table_insert(table, "Alice", 20);
    second = table_insert(table, "Bob", 21);

    assert(first != NULL);
    assert(second != NULL);
    assert(first->id == 1);
    assert(second->id == 2);

    table_destroy(table);
}

/* Verifies ID search uses the inserted indexed records. */
static void test_table_find_by_id(void) {
    Table *table = table_create();
    Record *alice;

    assert(table != NULL);

    alice = table_insert(table, "Alice", 20);
    assert(alice != NULL);
    assert(table_find_by_id(table, alice->id) == alice);
    assert(table_scan_by_id(table, alice->id) == alice);

    table_destroy(table);
}

/* Verifies non-ID lookups use simple linear scans. */
static void test_table_linear_search_fields(void) {
    Table *table = table_create();
    Record *alice;
    Record *bob;

    assert(table != NULL);

    alice = table_insert(table, "Alice", 20);
    bob = table_insert(table, "Bob", 30);

    assert(alice != NULL);
    assert(bob != NULL);
    assert(table_find_by_name(table, "Bob") == bob);
    assert(table_find_by_age(table, 20) == alice);

    table_destroy(table);
}

/* Verifies the SQL executor handles insert and select statements. */
static void test_sql_execution(void) {
    Table *table = table_create();
    SQLResult result;

    assert(table != NULL);

    result = sql_execute(table, "INSERT INTO users VALUES ('Alice', 20);");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_INSERT);
    assert(result.inserted_id == 1);

    result = sql_execute(table, "INSERT INTO users VALUES ('Bob', 30);");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_INSERT);
    assert(result.inserted_id == 2);

    result = sql_execute(table, "SELECT * FROM users;");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_SELECT_ALL);
    assert(result.row_count == 2);

    result = sql_execute(table, "SELECT * FROM users WHERE id = 1;");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_SELECT_ONE);
    assert(result.record != NULL);
    assert(strcmp(result.record->name, "Alice") == 0);

    result = sql_execute(table, "SELECT * FROM users WHERE name = 'Alice';");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_SELECT_ONE);
    assert(result.record != NULL);
    assert(result.record->age == 20);

    result = sql_execute(table, "SELECT * FROM users WHERE age = 20;");
    assert(result.status == SQL_STATUS_OK);
    assert(result.action == SQL_ACTION_SELECT_ONE);
    assert(result.record != NULL);
    assert(result.record->id == 1);

    result = sql_execute(table, "SELECT * FROM users WHERE id = 999;");
    assert(result.status == SQL_STATUS_NOT_FOUND);
    assert(result.action == SQL_ACTION_SELECT_ONE);
    assert(result.row_count == 0);

    result = sql_execute(table, "INSERT users VALUES ('Bad', 10);");
    assert(result.status == SQL_STATUS_SYNTAX_ERROR);

    table_destroy(table);
}

/* Runs all unit tests in a single executable. */
int main(void) {
    test_empty_tree_search();
    test_single_insert_search();
    test_duplicate_key_rejected();
    test_leaf_split_search();
    test_internal_split_new_root();
    test_leaf_next_link();
    test_table_auto_increment();
    test_table_find_by_id();
    test_table_linear_search_fields();
    test_sql_execution();

    printf("All unit tests passed.\n");
    return 0;
}
