#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <stdio.h>
#include <vector>
#include <atomic>

#define DEBUG
#ifdef DEBUG
#define dbg_printf(fmt, ...) printf("%s line:%d %s():" fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

using namespace std;



typedef struct tree_node_t
{
    struct tree_node_t *parent;
    struct tree_node_t *left_child;
    struct tree_node_t *right_child;
    int value;
    char color; // RED or BLACK
    bool is_leaf;
    atomic<bool> flag;
} tree_node;

#define RED 0
#define BLACK 1

/* function prototypes */
/* main functions */
tree_node *rb_init(void);
void right_rotate(tree_node *root, tree_node *node);
void left_rotate(tree_node *root, tree_node *node);
void tree_insert(tree_node *root, tree_node *node);
void rb_insert(tree_node *root, int value);
tree_node *get_remove_ndoe(tree_node *node);
void rb_remove(tree_node *root, int value);
void rb_remove_fixup(tree_node *root, tree_node *node);
tree_node *tree_search(tree_node *root, int value);

/* utility functions  */
void show_tree(tree_node *root);
bool check_tree(tree_node *root);
bool check_tree_dfs(tree_node *root);

bool is_root(tree_node *node);
bool is_left(tree_node *node);

tree_node *create_leaf_node(void);
void remove_node(tree_node *node);
tree_node *get_uncle(tree_node *node);
tree_node *get_right_min(tree_node *node);
int get_num_null(tree_node *node);
tree_node *replace_parent(tree_node *root, tree_node *node);

void free_node(tree_node *node);

/* lock-free functions */
bool setup_local_area_for_insert(tree_node *x);
tree_node *move_inserter_up(tree_node *oldx, vector<tree_node *> local_area);

inline void print_get(tree_node *x)
{
    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)x);
}

inline void print_release(tree_node *x)
{
    dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)x);
}
#endif
