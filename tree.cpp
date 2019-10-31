#include "tree.h"

#include<vector>

/**
 * check tree with BFS
 */
int check_tree(tree_node *root)
{
    // rule 1,3 inherently holds
    // rule 2
    if (root->color != BLACK)
    {
        fprintf(stderr, "[ERROR] tree root with non-black color\n");
        return 1;
    }
    
    std::vector<tree_node> frontier;
    frontier.clear();
    frontier.push_back(*root);

    while (frontier.size() > 0)
    {
        tree_node cur_node = frontier.back();
        tree_node *left_child = cur_node.left_child;
        tree_node *right_child = cur_node.right_child;

        // rule 4
        if (cur_node.color == RED)
        {
            if (left_child != NULL && left_child->color == RED)
            {
                fprintf(stderr, "[ERROR] red node's child must be black.\n");
                return 1;
            }
            if (right_child != NULL && right_child->color == RED)
            {
                fprintf(stderr, "[ERROR] red node's child must be black.\n");
                return 1;
            }
        }
        
        // rule 5
        if (cur_node.color == BLACK)
        {
            if (left_child != NULL && right_child != NULL)
            {
                if (left_child->color != right_child->color)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return 1;
                }
            }
            else if (left_child == NULL && right_child == NULL)
            {
                /* seems good */
            }
            else if (left_child == NULL)
            {
                /* right_child must be red and has two NULL children */
                if (right_child->color != RED)
                {
                    fprintf(stderr, 
                            "[ERROR] rule 5 violated.\n");
                    return 1;
                }
                if (right_child->left_child || right_child->right_child)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return 1;
                }
            }
            else if (right_child == NULL)
            {
                /* left_child must be red and has two NULL children */
                if (left_child->color != RED)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return 1;
                }
                if (left_child->left_child || left_child->right_child)
                {
                    fprintf(stderr,
                            "[ERROR] rule 5 violated.\n");
                    return 1;
                }
            }
        }

        frontier.pop_back();
        if (left_child)
        {
            frontier.push_back(*left_child);
        }
        if (right_child)
        {
            frontier.push_back(*right_child);
        }
    }

    return 0;
}

void insert(tree_node *root, int value)
{
    // TODO
}

void remove(tree_node *root, int value)
{
    // TODO
}

void search(tree_node *root, int value)
{
    // TODO
}
