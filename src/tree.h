#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <atomic>

// #define DEBUG
#ifdef DEBUG
#define dbg_printf(fmt, ...) do {\
        pthread_t         self;\
        self = pthread_self();\
        printf("tid:%u %s line:%d %s():" fmt, (unsigned)self, __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while(0)

#else
#define dbg_printf(...)
#endif

#define RED 0
#define BLACK 1

#define DEFAULT_MARKER -1

using namespace std;

typedef struct tree_node_t
{
    struct tree_node_t *parent;
    struct tree_node_t *left_child;
    struct tree_node_t *right_child;
    int value;
    char color; // RED or BLACK
    bool is_leaf;
    bool is_root;
    atomic<bool> flag;
    int marker;
} tree_node;

typedef struct move_up_struct_t
{
    tree_node *goal_node; // grandparent node
    vector<tree_node *> nodes_with_flag;
    int other_close_process_TID;
    bool valid;
} move_up_struct;

/* function prototypes */
/* main functions */
tree_node *rb_init(void);
void right_rotate(tree_node *root, tree_node *node);
void left_rotate(tree_node *root, tree_node *node);
void tree_insert(tree_node *root, tree_node *node);
void rb_insert(tree_node *root, int value);
void rb_remove(tree_node *root, int value);
tree_node *rb_remove_fixup(tree_node *root, tree_node *node);
tree_node *tree_search(tree_node *root, int value);

/* utility functions  */
tree_node *create_dummy_node(void);
void show_tree(tree_node *root);
bool check_tree(tree_node *root);
bool check_tree_dfs(tree_node *root);

bool is_root(tree_node *root, tree_node *node);
bool is_left(tree_node *node);

tree_node *get_remove_ndoe(tree_node *node);
tree_node *create_leaf_node(void);
void remove_node(tree_node *node);
tree_node *get_uncle(tree_node *node);
int get_num_null(tree_node *node);
tree_node *replace_parent(tree_node *root, tree_node *node);

void free_node(tree_node *node);
void clear_local_area(void);


/* lock-free functions */
bool is_in(tree_node *target_node, move_up_struct *target_move_up_struct);
bool is_goal(tree_node *target_node, move_up_struct *target_move_up_struct);
pthread_mutex_t *move_up_lock_init(int num_processes);
void release_flag(move_up_struct *target_move_up_struct, bool success,
                  tree_node *target_node);

// insert related
bool setup_local_area_for_insert(tree_node *x);
tree_node *move_inserter_up(tree_node *oldx, vector<tree_node *> &local_area);

// delete related
bool setup_local_area_for_delete(tree_node *y, tree_node *z);
bool apply_move_up_rule(tree_node *x, tree_node *w);
tree_node *move_deleter_up(tree_node *oldx);
tree_node *par_find(tree_node *root, int value);
tree_node *par_find_successor(tree_node *delete_node);
bool release_markers_above(tree_node *start, tree_node *z);
void clear_self_moveUpStruct(void);
void fix_up_case1(tree_node *x, tree_node *w);
void fix_up_case3(tree_node *x, tree_node *w);
void fix_up_case1_r(tree_node *x, tree_node *w); // mirror case
void fix_up_case3_r(tree_node *x, tree_node *w); // mirror case

inline void print_get(tree_node *x)
{
    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)x);
}

inline void print_release(tree_node *x)
{
    dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)x);
}
#endif
