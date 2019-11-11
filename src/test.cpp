#include "tree.h"

#include <stdlib.h>

// init

int main(int argc, char const *argv[])
{
    tree_node *root = rb_init();

    rb_insert(root, 3);
    rb_insert(root, 5);
    rb_insert(root, 15);
    rb_insert(root, 1);
    rb_insert(root, 2);
    rb_insert(root, 7);
    rb_insert(root, 50);
    rb_remove(root, 2);
    rb_remove(root, 7);
    rb_remove(root, 3);
    return 0;
}
