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
    root->is_leaf = false;
    root->parent = NULL;
    return root;
}

/**
 * perform red-black left rotation
 */
void left_rotate(tree_node *node)
{
    if (node->is_leaf)
    {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        return;
    }
    
    if (node->right_child->is_leaf)
    {
        fprintf(stderr, 
                "[ERROR] invalid rotate on node with NULL right child.\n");
        return;
    }

    tree_node *right_child = node->right_child;
    right_child->parent = node->parent;
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
void right_rotate(tree_node *node)
{
    if (node->is_leaf)
    {
        fprintf(stderr, "[ERROR] invalid rotate on NULL node.\n");
        return;
    }

    if (node->left_child->is_leaf)
    {
        fprintf(stderr,
                "[ERROR] invalid rotate on node with NULL left child.\n");
        return;
    }

    tree_node *left_child = node->left_child;
    left_child->parent = node->parent;
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

    if (root->left_child->is_leaf)
    {
        free_node(root->left_child);
        root->left_child = new_node;
        dbg_printf("[Insert] new node with value (%d)\n", value);
        return;
    }
    
    // insert like any binary search tree
    tree_node *curr_node = root->left_child;
    while (curr_node)
    {
        if (value > curr_node->value) /* go right */
        {
            if (curr_node->right_child->is_leaf)
            {
                curr_node->right_child = new_node;
                break;
            }
            
            curr_node = curr_node->right_child;
        }
        else /* go left */
        {
            if (curr_node->left_child->is_leaf)
            {
                curr_node->left_child = new_node;
                break;
            }

            curr_node = curr_node->left_child;
        }
    }

    dbg_printf("[Insert] new node with value (%d)\n", value);
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
    new_node->is_leaf = false;
    new_node->parent = NULL;

    tree_insert(root, new_node); // normal insert

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

        if (is_root(parent)) // trivial case 2
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
                left_rotate(parent);
                curr_node = parent;
            case true:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                right_rotate(parent->parent);
                break;
            }
        }
        else // parent is the right child of its parent
        {
            switch (is_left(curr_node))
            {
            case true:
                right_rotate(parent);
                curr_node = parent;
            case false:
                parent = curr_node->parent;
                uncle = get_uncle(curr_node);

                parent->parent->color = RED;
                parent->color = BLACK;
                left_rotate(parent->parent);
                break;
            }
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
    dbg_printf("[Remove] node to delete with value %d.\n", delete_node->value);
    replace_node = replace_parent(root, delete_node);

    if (delete_node->color == BLACK) /* fixup case */
    {
        rb_remove_fixup(root, replace_node);
    }
    dbg_printf("[Remove] node with value %d complete.\n", delete_node->value);
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
            if (brother_node->color == RED)
            {
                brother_node->color = BLACK;
                node->parent->color = RED;
                left_rotate(node->parent);
                brother_node = node->parent->right_child;
            }

            else if (brother_node->left_child->color == BLACK &&
                brother_node->right_child->color == BLACK)
            {
                brother_node->color = RED;
                node = node->parent;
            }
            else if (brother_node->right_child->color == BLACK)
            {
                brother_node->left_child->color = BLACK;
                brother_node->color = RED;
                right_rotate(brother_node);
                brother_node = node->parent->right_child;

                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->right_child->color = BLACK;
                left_rotate(node->parent);
                break;
            }
        }
        else
        {
            brother_node = node->parent->left_child;
            if (brother_node->color == RED)
            {
                brother_node->color = BLACK;
                node->parent->color = RED;
                right_rotate(node->parent);
                brother_node = node->parent->left_child;
            }

            else if (brother_node->left_child->color == BLACK &&
                     brother_node->right_child->color == BLACK)
            {
                brother_node->color = RED;
                node = node->parent;
            }
            else if (brother_node->left_child->color == BLACK)
            {
                brother_node->right_child->color = BLACK;
                brother_node->color = RED;
                left_rotate(brother_node);
                brother_node = node->parent->left_child;

                brother_node->color = node->parent->color;
                node->parent->color = BLACK;
                brother_node->left_child->color = BLACK;
                right_rotate(node->parent);
                break;
            }
        }
    }
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
