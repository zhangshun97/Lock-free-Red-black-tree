#include "tree.h"

#include <stdlib.h>

// init

int main(int argc, char const *argv[])
{
    tree_node *root = rb_init();

    rb_insert(root, 3);
    rb_insert(root, 5);
    show_tree(root);
    rb_insert(root, 15);
    show_tree(root);
    rb_insert(root, 1);
    show_tree(root);
    rb_insert(root, 2);
    show_tree(root);
    rb_insert(root, 7);
    show_tree(root);
    rb_insert(root, 50);
    show_tree(root);
    rb_remove(root, 2);
    rb_remove(root, 7);
    rb_remove(root, 3);
    return 0;
}
