#include "tree.h"

#include <stdlib.h>
#include <vector>

/******************
 * helper function
 ******************/

/**
 * check tree with BFS
 */
bool check_tree(tree_node *root)
{
    // rule 1,3 inherently holds
    // rule 2
    root = root->left_child;
    if (root->color != BLACK)
    {
        fprintf(stderr, "[ERROR] tree root with non-black color\n");
        return false;
    }

    std::vector<tree_node*> frontier;
    frontier.clear();
    frontier.push_back(root);

    while (frontier.size() > 0)
    {
        tree_node *cur_node = frontier.back();
        tree_node *left_child = cur_node->left_child;
        tree_node *right_child = cur_node->right_child;

        // rule 4
        if (cur_node->color == RED)
        {
            if (left_child->color == RED)
            {
                fprintf(stderr, "[ERROR] red node's child must be black.\n");
                return false;
            }
            if (right_child->color == RED)
            {
                fprintf(stderr, "[ERROR] red node's child must be black.\n");
                return false;
            }
        }

        // rule 5
        if (cur_node->color == BLACK)
        {
            if (!left_child->is_leaf && !right_child->is_leaf)
            {
                if (left_child->color != right_child->color)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return false;
                }
            }
            else if (left_child->is_leaf && right_child->is_leaf)
            {
                /* seems good */
            }
            else if (left_child->is_leaf)
            {
                /* right_child must be red and has two NULL children */
                if (right_child->color != RED)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return false;
                }
                if (right_child->left_child || right_child->right_child)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return false;
                }
            }
            else if (right_child->is_leaf)
            {
                /* left_child must be red and has two NULL children */
                if (left_child->color != RED)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return false;
                }
                if (left_child->left_child || left_child->right_child)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return false;
                }
            }
        }

        frontier.pop_back();
        if (!left_child->is_leaf)
        {
            frontier.push_back(left_child);
        }
        if (!right_child->is_leaf)
        {
            frontier.push_back(right_child);
        }
    }

    return true;
}

/**
 * true if node is the left child of its parent node
 */
bool is_root(tree_node *node)
{
    if (node->parent == NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * true if node is the left child of its parent node
 */
tree_node* create_leaf_node(void)
{
    tree_node *new_node;
    new_node = (tree_node *)malloc(sizeof(tree_node));
    new_node->color = BLACK;
    new_node->value = 0;
    new_node->left_child = NULL;
    new_node->right_child = NULL;
    new_node->is_leaf = true;
    return new_node;
}

/**
 * replace the parent node with its successor
 */
tree_node *replace_parent(tree_node *root, tree_node *node)
{
    if (get_num_null(node) != 1)
    {
        fprintf(stderr, "[ERROR] replace parent error non-null child.\n");
        return NULL;
    }

    tree_node *child;
    if (node->left_child->is_leaf)
    {
        child = node->right_child;
        free_node(node->left_child);
    }
    else
    {
        child = node->left_child;
        free_node(node->right_child);
    }
    
    if (is_root(node))
    {
        child->parent = NULL;
        root->left_child = child;
    }
    
    else if (is_left(node))
    {
        child->parent = node->parent;
        node->parent->left_child = child;
    }

    else
    {
        child->parent = node->parent;
        node->parent->right_child = child;
    }

    return child;
}

/**
 * true if node is the left child of its parent node
 * false if it's right child
 */
bool is_left(tree_node *node)
{
    if (node->parent->is_leaf)
    {
        fprintf(stderr, "[ERROR] root node has no parent.\n");
    }

    tree_node *parent = node->parent;
    if (node == parent->left_child)
    {
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * get the uncle node of current node
 */
tree_node *get_uncle(tree_node *node)
{
    if (node->parent->is_leaf)
    {
        fprintf(stderr, "[ERROR] get_uncle node should be at least layer 3.\n");
        return NULL;
    }

    if (node->parent->parent->is_leaf)
    {
        fprintf(stderr, "[ERROR] get_uncle node should be at least layer 3.\n");
        return NULL;
    }

    tree_node *parent = node->parent;
    tree_node *grand_parent = parent->parent;
    if (parent == grand_parent->left_child)
    {
        return grand_parent->right_child;
    }
    else
    {
        return grand_parent->left_child;
    }
}

/**
 * get the number of NULL child
 */
int get_num_null(tree_node *node)
{
    int ret = 0;

    if (node->left_child->is_leaf)
        ret++;
    if (node->right_child->is_leaf)
        ret++;

    return ret;
}

/**
 * get the minimum node within target node's right sub-tree
 * and put its value into the node that will be deleted
 */
tree_node *get_right_min(tree_node *node)
{
    if (node->right_child->is_leaf)
    {
        return NULL;
    }

    tree_node *curr_node = node->right_child;
    while (curr_node->left_child)
    {
        curr_node = curr_node->left_child;
    }

    // replace the target node with right min node
    // only the value
    node->value = curr_node->value;
    return curr_node;
}

/**
 * free current node
 */
void free_node(tree_node *node)
{
    free(node);
}
