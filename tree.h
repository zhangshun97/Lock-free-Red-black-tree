#ifndef RED_BLACK_TREE_H
#define RED_BLACK_TREE_H

#include<stdio.h>

#define DEBUG
#ifdef DEBUG
#define dbg_printf(...) printf(__VA_ARGS__)
#else
#define dbg_printf(...)
#endif

typedef struct tree_node_t
{
    struct tree_node_t *parent;
    struct tree_node_t *left_child;
    struct tree_node_t *right_child;
    int value;
    char color; // RED or BLACK
} tree_node;

#define RED 0
#define BLACK 1


/* function prototypes */
int check_tree(tree_node root);
void insert(tree_node *root, int value);
void remove(tree_node *root, int value);
void search(tree_node *root, int value);

#endif
