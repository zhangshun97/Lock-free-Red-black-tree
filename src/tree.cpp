#include "tree.h"

#include <stdlib.h>

// bool expected = false;

/**
 * initialize red-black tree and return its root
 */
tree_node *rb_init(void)
{
    tree_node *root;
    root = (tree_node *)malloc(sizeof(tree_node));
    root->color = BLACK;
    root->value = INT32_MAX;
    root->left_child = create_leaf_node();
    root->right_child = create_leaf_node();
    root->is_leaf = false;
    root->parent = NULL;
    root->flag = false;
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

    // insert like any binary search tree
    bool expected = false;
    while (!root->flag.compare_exchange_weak(expected, true));

    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)root);

    // empty tree
    if (root->left_child->is_leaf)
    {
        free_node(root->left_child);
        new_node->flag = true;
        dbg_printf("[FLAG] set flag of %lu\n", (unsigned long)new_node);
        root->left_child = new_node;
        dbg_printf("[Insert] new node with value (%d)\n", value);
        dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)root);
        root->flag = false;
        return;
    }

    // release root's flag for non-empty tree
    dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)root);
    root->flag = false;

    restart:

    tree_node *z = NULL;
    tree_node *curr_node = root->left_child;
    expected = false;
    if (!curr_node->flag.compare_exchange_strong(expected, true))
    {
        dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)curr_node);
        
        goto restart;
    }
    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)curr_node);

    
    while (!curr_node->is_leaf)
    {
        z = curr_node;
        if (value > curr_node->value) /* go right */
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
        else /* go left */
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
        }

        expected = false;
        if (!curr_node->flag.compare_exchange_weak(expected, true))
        {
            dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)curr_node);
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)z);
            z->flag = false;// release z's flag
            
            goto restart;
        }

        dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)curr_node);

        if (!curr_node->is_leaf)
        {
            // release old curr_node's flag
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)z);
            z->flag = false;
        }
    }
    
    new_node->flag = true;
    if (!setup_local_area_for_insert(z))
    {
        curr_node->flag = false;
        dbg_printf("[FLAG] release flag of %lu and %lu\n", (unsigned long)z, (unsigned long)curr_node);
        z->flag = false;
        goto restart;
    }

    // now the local area has been setup
    // insert the node
    new_node->parent = z;
    if (z == root)
    {
        free(root->left_child);
        root->left_child = new_node;
        new_node->parent = NULL;
    }
    else if (value < z->value)
    {
        free(z->left_child);
        z->left_child = new_node;
    }
    else if (value > z->value)
    {
        free(z->right_child);
        z->right_child = new_node;
    }
    
    dbg_printf("[Insert] new node with value (%d)\n", value);

    // dbg_printf("[Insert] failed\n");
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
    
    tree_node *parent, *uncle = NULL, *grandparent = NULL;

    parent = curr_node->parent;
    vector<tree_node *> local_area = {curr_node, parent};

    if (parent != NULL)
    {
        grandparent = parent->parent; 
    }

    if (grandparent != NULL)
    {
        if (grandparent->left_child == parent)
        {
            uncle = grandparent->right_child;
        }
        else
        {
            uncle = grandparent->left_child;
        }
    }

    local_area.push_back(uncle);
    local_area.push_back(grandparent);

    if (is_root(curr_node))
    {
        curr_node->color = BLACK;
        for (auto node:local_area)
        {
            if (node != NULL) 
            {
                dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)node);
                node->flag = false;
            }
        }
        dbg_printf("[INSERT] insertFixup complete.\n");
        return;
    }

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
            // curr_node = parent->parent;
            parent->parent->color = RED;

            curr_node = move_inserter_up(curr_node, local_area);
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

    // release flags of all nodes in local_area
    for (auto node : local_area)
    {
        if (node != NULL)
        {
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)node);
            node->flag = false;
        }
    }
    
    dbg_printf("[Insert] rb fixup complete.\n");
}

bool setup_local_area_for_insert(tree_node *x) {
    tree_node *parent = x->parent;
    tree_node *uncle = NULL;

    if (parent == NULL) return true;
    
    // if (!x->flag.compare_exchange_strong(expected, true))
    // {
    //     return false;
    // }

    // print_get(x);

    bool expected = false;
    if (!parent->flag.compare_exchange_weak(expected, true))
    {
        dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)parent);
        return false;
    }

    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)parent);
    
    // fail when parent of x changes
    if (parent != x->parent)
    {
        dbg_printf("[FLAG] parent changed from %lu to %lu\n", (unsigned long)parent, (unsigned long)parent);
        dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)parent);
        parent->flag = false;
        return false;
    }

    if (x == x->parent->left_child)
    {
        uncle = x->parent->right_child;
    }
    else
    {
        uncle = x->parent->left_child;
    }

    expected = false;
    if (!uncle->flag.compare_exchange_weak(expected, true))
    {
        dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)uncle);
        dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)x->parent);
        x->parent->flag = false;
        return false;
    }

    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)uncle);

    // now the process has the flags of x, x's parent and x's uncle
    return true;
}

tree_node *move_inserter_up(tree_node *oldx, vector<tree_node *> &local_area)
{
    tree_node *oldp = oldx->parent;
    tree_node *oldgp = oldp->parent;
    // tree_node *uncle = NULL;
    // if (oldp == oldgp->left_child)
    // {
    //     uncle = oldgp->right_child;
    // }
    // else
    // {
    //     uncle = oldgp->left_child;
    // }

    bool expected = false;

    tree_node *newx, *newp = NULL, *newgp = NULL, *newuncle = NULL;
    newx = oldgp;
    while(true && newx->parent != NULL)
    {
        newp = newx->parent;
        expected = false;
        if (!newp->flag.compare_exchange_weak(expected, true))
        {
            dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)newp);
            continue;
        }

        dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)newp);

        newgp = newp->parent;
        if (newgp == NULL) break;
        expected = false;
        if (!newgp->flag.compare_exchange_weak(expected, true))
        {
            dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)newp);
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)newgp);

        if (newp == newgp->left_child)
        {
            newuncle = newgp->right_child;
        }
        else
        {
            newuncle = newgp->left_child;
        }

        expected = false;
        if (!newuncle->flag.compare_exchange_weak(expected, true))
        {
            dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)newuncle);
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)newp);
            newgp->flag = false;
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)newuncle);

        // now the process has the flags of newp, newgp and newuncle
        break;
    }

    local_area.push_back(newx);
    local_area.push_back(newp);
    local_area.push_back(newgp);
    local_area.push_back(newuncle);

    return newx;
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
