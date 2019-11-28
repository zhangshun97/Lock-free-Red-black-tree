#include "tree.h"

#include <stdlib.h>

/**
 * initialize red-black tree and return its root
 */
tree_node *rb_init(void)
{
    tree_node *root;
    root = (tree_node *)malloc(sizeof(tree_node));
    root->color = BLACK;
    root->value = 0;
    root->left_child = create_leaf_node();
    root->right_child = create_leaf_node();
    parent(left(root)) = root;
    parent(right(root)) = root;
    root->is_leaf = false;
    root->parent = NULL;
    pthread_mutex_init(&root->read_write_lock, NULL);
    pthread_mutex_init(&root->exclusive_lock, NULL);
    return root;
}

/**
 * perform red-black left rotation
 */
void left_rotate(tree_node *root, tree_node *node)
{
    if (node->is_leaf)
    {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        exit(1);
    }
    
    if (node->right_child->is_leaf)
    {
        fprintf(stderr, 
                "[ERROR] invalid rotate on node with NULL right child.\n");
        exit(1);
    }

    tree_node *right_child = node->right_child;
    right_child->parent = node->parent;
    if (is_root(node))
    {
        root->left_child = right_child;
    }
    else if (is_left(node))
    {
        node->parent->left_child = right_child;
    }
    else
    {
        node->parent->right_child = right_child;
    }

    node->parent = right_child;

    node->right_child = right_child->left_child;
    right_child->left_child = node;

    if (node->right_child)
    {
        node->right_child->parent = node;
    }
    
    dbg_printf("[Rotate] Left rotation complete.\n");
}

/**
 * perform red-black right rotation
 */
void right_rotate(tree_node *root, tree_node *node)
{
    if (node->is_leaf)
    {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        exit(1);
    }

    if (node->left_child->is_leaf)
    {
        fprintf(stderr,
                "[ERROR] invalid rotate on node with NULL left child.\n");
        exit(1);
    }

    tree_node *left_child = node->left_child;
    left_child->parent = node->parent;
    if (is_root(node))
    {
        root->left_child = left_child;
    }
    else if (is_left(node))
    {
        node->parent->left_child = left_child;
    }
    else
    {
        node->parent->right_child = left_child;
    }
    
    node->parent = left_child;

    node->left_child = left_child->right_child;
    left_child->right_child = node;

    if (node->left_child)
    {
        node->left_child->parent = node;
    }

    dbg_printf("[Rotate] Right rotation complete.\n");
}

/**
 * basic insertion of a binary search tree
 * further fixup needed for red-black tree
 */
void tree_insert(tree_node *root, tree_node *new_node)
{
    int value = new_node->value;

    // dbg_printf("[Insert] before getting root's write lock.\n");
    get_write_lock(root);
    // pthread_mutex_lock(&root->read_write_lock);
    dbg_printf("[Insert] get root's write lock.\n");

    if (root->left_child->is_leaf)
    {
        free_node(root->left_child);
        root->left_child = new_node;
        set_black(new_node);
        dbg_printf("[Insert] new node with value (%d)\n", value);
        release_write_lock(root);
        return;
    }

    release_write_lock(root);
    dbg_printf("[Insert] release root's write lock.\n");

    // free(new_node);
    
    // insert like any binary search tree
    tree_node *curr_node = root->left_child;
    bool found = 0;
    get_write_lock(curr_node);
    dbg_printf("[Insert] get write lock of %lu.\n", (unsigned long)curr_node);
    tree_node *locked_node = curr_node;
    while (!curr_node->is_leaf && !found)
    {
        tree_node *parent = curr_node;
        if (value > curr_node->value)       /* go right */
        {
            // if (curr_node->right_child->is_leaf)
            // {
            //     free_node(curr_node->right_child);
            //     curr_node->right_child = new_node;
            //     new_node->parent = curr_node;
            //     dbg_printf("[Insert] new node with value (%d)\n", value);
            //     return;
            // }
            
            curr_node = curr_node->right_child;
        }
        else if (value < curr_node->value)  /* go left */
        {
            // if (curr_node->left_child->is_leaf)
            // {
            //     free_node(curr_node->left_child);
            //     curr_node->left_child = new_node;
            //     new_node->parent = curr_node;
            //     dbg_printf("[Insert] new node with value (%d)\n", value);
            //     return;
            // }

            curr_node = curr_node->left_child;
        } else {                            /* found */
            found = 1;
        }

        dbg_printf("curr_node: %lu\n", (unsigned long) curr_node);

        if (is_black(curr_node) && is_black(parent) && parent != locked_node)
        {
            get_write_lock(parent);
            dbg_printf("[Insert] get write lock of %lu.\n", (unsigned long)parent);
            release_write_lock(locked_node);
            dbg_printf("[Insert] release write lock of %lu.\n", (unsigned long)locked_node);
            locked_node = parent;
        }
    }

    if (!found) /* curr_node is a leaf */
    {
        get_exclusive_lock(curr_node);
        color(curr_node) = RED;
        curr_node->value = value;
        curr_node->is_leaf = false;
        left(curr_node) = create_leaf_node();
        right(curr_node) = create_leaf_node();
        parent(left(curr_node)) = curr_node;
        parent(right(curr_node)) = curr_node;
        set_red(curr_node);
        release_exclusive_lock(curr_node);
        while(curr_node != root->left_child && is_red(parent(curr_node)))
        {
            tree_node *parent = parent(curr_node);
            tree_node *gparent = parent(parent);
            tree_node *aunt = NULL;
            if (parent == left(gparent))
            {
                aunt = right(gparent);
                if (is_red(aunt))
                {
                    set_black(aunt);
                    set_black(parent);
                    set_red(gparent);
                    curr_node = gparent;
                }
                else if (curr_node == left(parent))
                {
                    set_black(parent);
                    set_red(gparent);
                    tree_node *sister = right(parent);

                    // Perform rotation
                    if (gparent == root->left_child)
                    {   /* need to change root */
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(sister);

                        root->left_child = parent;
                        parent(parent) = root;
                        right_child(parent, gparent);
                        left_child(gparent, sister);

                        release_exclusive_lock(sister);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                    }
                    else
                    {   /* no need to change root */
                        tree_node *ggparent = parent(gparent);
                        get_exclusive_lock(ggparent);
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(sister);
                        if (gparent == left(ggparent))
                        {
                            left_child(ggparent, parent);
                        }
                        else
                        {
                            right_child(ggparent, parent);
                        }
                        right_child(parent, gparent);
                        left_child(gparent, sister);

                        release_exclusive_lock(sister);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                        release_exclusive_lock(ggparent);
                    }
                }
                else
                {   /* curr_node is right child */
                    set_black(curr_node);
                    set_red(gparent);
                    tree_node *left = left(curr_node), *right = right(curr_node);
                    if (gparent == root->left_child)
                    {   /* need to change root */
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(curr_node);
                        get_exclusive_lock(left);
                        get_exclusive_lock(right);

                        root->left_child = curr_node;
                        left_child(curr_node, parent);
                        right_child(curr_node, gparent);
                        right_child(parent, left);
                        left_child(gparent, right);

                        release_exclusive_lock(right);
                        release_exclusive_lock(left);
                        release_exclusive_lock(curr_node);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                    }
                    else
                    {   /* no need to change root */
                        tree_node *ggparent = parent(gparent);
                        get_exclusive_lock(ggparent);
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(curr_node);
                        get_exclusive_lock(left);
                        get_exclusive_lock(right);

                        if (gparent == left(ggparent))
                        {
                            left_child(ggparent, curr_node);
                        }
                        else
                        {
                            right_child(ggparent, curr_node);
                        }
                        left_child(curr_node, parent);
                        right_child(curr_node, gparent);
                        right_child(parent, left);
                        left_child(gparent, right);

                        release_exclusive_lock(right);
                        release_exclusive_lock(left);
                        release_exclusive_lock(curr_node);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                        release_exclusive_lock(ggparent);
                    }
                }
            }
            else
            {
                aunt = left(gparent);
                if (is_red(aunt))
                {
                    set_black(aunt);
                    set_black(parent);
                    set_red(gparent);
                    curr_node = gparent;
                }
                else if (curr_node == right(parent))
                {
                    set_black(parent);
                    set_red(gparent);
                    tree_node *sister = left(parent);
                    if (gparent == root->left_child)
                    {
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(sister);
                        root->left_child = parent;
                        left_child(parent, gparent);
                        right_child(gparent, sister);
                        release_exclusive_lock(sister);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                    }
                    else
                    {
                        tree_node *ggparent = parent(gparent);
                        get_exclusive_lock(ggparent);
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(sister);
                        if (gparent == left(ggparent))
                        {
                            left_child(ggparent, parent);
                        }
                        else
                        {
                            right_child(ggparent, parent);
                        }
                        left_child(parent, gparent);
                        right_child(gparent, sister);

                        release_exclusive_lock(sister);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                        release_exclusive_lock(ggparent);
                    }
                }
                else
                {   /* node is left child */
                    set_black(curr_node);
                    set_red(gparent);
                    tree_node *left = left(curr_node);
                    tree_node *right = right(curr_node);
                    if (gparent == root->left_child)
                    {
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(curr_node);
                        get_exclusive_lock(left);
                        get_exclusive_lock(right);

                        root->left_child = curr_node;
                        right_child(curr_node, parent);
                        left_child(curr_node, gparent);
                        left_child(parent, right);
                        right_child(gparent, left);

                        release_exclusive_lock(right);
                        release_exclusive_lock(left);
                        release_exclusive_lock(curr_node);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                    }
                    else
                    {   /* no need to change root */
                        tree_node *ggparent = parent(gparent);
                        get_exclusive_lock(ggparent);
                        get_exclusive_lock(gparent);
                        get_exclusive_lock(parent);
                        get_exclusive_lock(curr_node);
                        get_exclusive_lock(left);
                        get_exclusive_lock(right);

                        if (gparent == left(ggparent))
                        {
                            left_child(ggparent, curr_node);
                        }
                        else
                        {
                            right_child(ggparent, curr_node);
                        }
                        right_child(curr_node, parent);
                        left_child(curr_node, gparent);
                        left_child(parent, right);
                        right_child(gparent, left);
                        
                        release_exclusive_lock(right);
                        release_exclusive_lock(left);
                        release_exclusive_lock(curr_node);
                        release_exclusive_lock(parent);
                        release_exclusive_lock(gparent);
                        release_exclusive_lock(ggparent);
                    }
                }
            }
        }
    }

    set_black(root->left_child);
    release_write_lock(locked_node);
    dbg_printf("[Insert] Completed.\n");
}

/**
 * insert a new node
 * fixup the tree to be a red-black tree
 */
void rb_insert(tree_node *root, int value)
{
    tree_node *new_node;
    new_node = (tree_node *)malloc(sizeof(tree_node));
    new_node->color = RED;
    new_node->value = value;
    new_node->left_child = create_leaf_node();
    new_node->right_child = create_leaf_node();
    parent(left(new_node)) = new_node;
    parent(right(new_node)) = new_node;
    new_node->is_leaf = false;
    new_node->parent = NULL;
    pthread_mutex_init(&new_node->read_write_lock, NULL);
    pthread_mutex_init(&new_node->exclusive_lock, NULL);

    tree_insert(root, new_node); // normal insert

    return;
    tree_node *curr_node = new_node;
    tree_node *parent, *uncle;

    while (true)
    {
        if (is_root(curr_node)) // trivial case 1
        {
            curr_node->color = BLACK;
            break;
        }
        
        parent = curr_node->parent;

        if (parent->color == BLACK) // trivial case 2
        {
            break;
        }
        
        uncle = get_uncle(curr_node);

        if (parent->color == RED && uncle->color == RED) /* case 1 */
        {
            parent->color = BLACK;
            uncle->color = BLACK;
            curr_node = parent->parent;
            curr_node->color = RED;

            continue;
        }

        if (is_left(parent))
        {
            switch (is_left(curr_node))
            {
            case false:
                left_rotate(root, parent);
                curr_node = parent;
            case true:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                right_rotate(root, parent->parent);
                break;
            }
            break;
        }
        else // parent is the right child of its parent
        {
            switch (is_left(curr_node))
            {
            case true:
                right_rotate(root, parent);
                curr_node = parent;
            case false:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                left_rotate(root, parent->parent);
                break;
            }
            break;
        }
    }
    
    dbg_printf("[Insert] rb fixup complete.\n");
}

/**
 * standard binary search tree remove
 */
tree_node *get_remove_ndoe(tree_node *node)
{
    if (node->is_leaf)
    {
        fprintf(stderr, "[ERROR] node not found.\n");
        return NULL;
    }

    int case_num = get_num_null(node);
    if (case_num == 0)
    {
        // replace value and remove successor
        tree_node *right_min_node = get_right_min(node);
        if (right_min_node->is_leaf)
            fprintf(stderr, "[ERROR] right min node error.\n");
        
        // replace the value
        node->value = right_min_node->value;
        // remove right min node
        return get_remove_ndoe(right_min_node);
    }
    else
    {
        return node;
    }
}

/**
 * red-black tree remove
 */
void rb_remove(tree_node *root, int value)
{
    tree_node *node = tree_search(root, value);
    tree_node *delete_node, *replace_node;
    delete_node = get_remove_ndoe(node);
    dbg_printf("[Remove] node to delete with value %d.\n", value);
    replace_node = replace_parent(root, delete_node);

    if (delete_node->color == BLACK) /* fixup case */
    {
        if (replace_node->color == RED)
        {
            replace_node->color = BLACK;
        }
        else
        {
            rb_remove_fixup(root, replace_node);
        }
    }
    dbg_printf("[Remove] node with value %d complete.\n", value);
    free_node(delete_node);
}

/**
 * red-black tree remove fixup
 * when delete node and replace node are both black
 * rule 5 is violated and should be fixed
 */
void rb_remove_fixup(tree_node *root, tree_node *node)
{
    while (!is_root(node) && node->color == BLACK)
    {
        tree_node *brother_node;
        if (is_left(node))
        {
            brother_node = node->parent->right_child;
            if (brother_node->color == RED) // case 1
            {
                brother_node->color = BLACK;
                node->parent->color = RED;
                left_rotate(root, node->parent);
                brother_node = node->parent->right_child; // must be black node
            } // case 1 will definitely turn into case 2

            if (brother_node->left_child->color == BLACK &&
                brother_node->right_child->color == BLACK) // case 2
            {
                brother_node->color = RED;
                node = node->parent;
                if (node->color == RED)
                {
                    node->color = BLACK;
                    break;
                }
            }
            
            else if (brother_node->right_child->color == BLACK) // case 3
            {
                brother_node->left_child->color = BLACK;
                brother_node->color = RED;
                right_rotate(root, brother_node);
                brother_node = node->parent->right_child;
            }

            else // case 4
            {
                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->right_child->color = BLACK;
                left_rotate(root, node->parent);
                break;
            }
        }
        else // mirror case of the above
        {
            brother_node = node->parent->left_child;
            if (brother_node->color == RED)
            {
                brother_node->color = BLACK;
                node->parent->color = RED;
                right_rotate(root, node->parent);
                brother_node = node->parent->left_child;
            }

            if (brother_node->left_child->color == BLACK &&
                     brother_node->right_child->color == BLACK)
            {
                brother_node->color = RED;
                node = node->parent;
                if (node->color == RED)
                {
                    node->color = BLACK;
                    break;
                }
            }

            else if (brother_node->left_child->color == BLACK)
            {
                brother_node->right_child->color = BLACK;
                brother_node->color = RED;
                left_rotate(root, brother_node);
                brother_node = node->parent->left_child;
            }

            else // case 4
            {
                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->left_child->color = BLACK;
                right_rotate(root, node->parent);
                break;
            }
        }
    }
    dbg_printf("[Remove] fixup complete.\n");
}

/**
 * standard binary tree search
 */
tree_node *tree_search(tree_node *root, int value)
{
    tree_node *curr_node = root->left_child;
    while (!curr_node->is_leaf)
    {
        if (curr_node->value == value)
        {
            return curr_node;
        }
        else if (value < curr_node->value)
        {
            curr_node = curr_node->left_child;
        }
        else
        {
            curr_node = curr_node->right_child;
        }
    }
    
    dbg_printf("[Warning] tree serach not found.\n");
    return curr_node;
}
