#include "tree.h"

#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <sys/types.h>
#include <unistd.h>

/******************
 * helper function
 ******************/

typedef struct tree_node_t
{
    struct tree_node_t *parent;
    struct tree_node_t *left_child;
    struct tree_node_t *right_child;
    int value;
    char color; // RED or BLACK
    bool is_leaf;
    atomic<bool> flag;
    int marker;
} tree_node;

typedef struct move_up_struct_t
{
    tree_node *goal_node; // grandparent node

    tree_node *pos1;
    tree_node *pos2;
    tree_node *pos3;
    tree_node *pos4;

    int other_close_process_PID;
    bool valid;
} move_up_struct;

/* global variables */
extern move_up_struct *move_up_list;
extern pthread_mutex_t *move_up_lock_list;

/**
 * initialize the lock list for moveUpStruct list
 */
pthread_mutex_t *move_up_lock_init(int num_processes)
{
    pthread_mutex_t *move_up_lock_list =
        (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_processes);
    for (size_t i = 0; i < num_processes; i++)
    {
        pthread_mutex_init(&move_up_lock_list[i], NULL);
    }
    return move_up_lock_list;
}

/**
 * releasr flags unless node is in move up struct
 * TODO: check if this is needed?
 */
void release_flags(move_up_struct *target_move_up_struct, 
                   bool success,
                   vector<tree_node *>nodes_to_release)
{
    for (auto target_node : nodes_to_release)
    {
        if (success)
        {
            if (!is_in(target_node, target_move_up_struct))
                target_node->flag = false;
            else // target node is in the inherited local area
            {
                if (is_goal(target_node, target_move_up_struct))
                {
                    // TODO: release unneeded flags in moveUpStruct 
                    // and discard moveUpStruct
                }
            }
        }
        else // release flag after failing to move up
        {
            if (!is_in(target_node, target_move_up_struct))
            {
                target_node->flag = false;
            }
        }
    }
}

/**
 * releasr flags unless node is in move up struct
 * TODO: check if this is good enough?
 */
void release_flag(move_up_struct *target_move_up_struct,
                  bool success,
                  tree_node *target_node)
{
    if (success)
    {
        if (!is_in(target_node, target_move_up_struct))
            target_node->flag = false;
        else // target node is in the inherited local area
        {
            if (is_goal(target_node, target_move_up_struct))
            {
                // TODO: release unneeded flags in moveUpStruct
                // and discard moveUpStruct
            }
        }
    }
    else // release flag after failing to move up
    {
        if (!is_in(target_node, target_move_up_struct))
        {
            target_node->flag = false;
        }
    }
}

/**
 * return true if spacing rule is satisfied
 */
bool spacing_rule_is_satisfied(tree_node *t, tree_node *z,
                                int PID_to_ignore,
                                move_up_struct *self_move_up_struct)
{
    // We hold flags on both t and z.
    // check that t has no marker set
    if (t != z && t->marker != 0) return false;

    // check that t’s parent has no flag or marker
    tree_node *tp = t->parent;
    if (tp != z)
    {
        if ((!is_in(tp, self_move_up_struct)) 
            && (!tp->flag.compare_exchange_weak(false, true)))
        {
            return false;
        }
        if (tp != t->parent) // verify that parent is unchanged
        {  
            tp->flag = false;
            return false;
        }
        if (tp->marker != 0)
        {
            tp->flag = false;
            return false;
        }
    }
    // check that t’s sibling has no flag or marker
    tree_node *ts = tp->left_child;
    if (is_left(t))
        ts = tp->right_child;
    
    if ((!is_in(ts, self_move_up_struct)) 
        && (!ts->flag.compare_exchange_weak(false, true)))
    {
        if (tp != z)
            release_flag(self_move_up_struct, false, tp);
        return false;
    }
    if ((ts->marker != 0) && (ts->marker != PID_to_ignore))
    {
        release_flag(self_move_up_struct, false, ts);
        if (tp != z)
            release_flag(self_move_up_struct, false, tp);
        return false;
    }
    if (tp != z)
        release_flag(self_move_up_struct, false, tp);
    release_flag(self_move_up_struct, false, ts);
    return true;
}

/**
 * Move Up Rule
 * 
 * @params
 *  x: target node
 *  w: sibling node
 */
bool apply_move_up_rule(tree_node *x, tree_node *w)
{
    bool case_hit = false;
    if (w->marker == w->parent->marker 
        && w->marker == w->right_child->marker 
        && w->marker != 0 
        && w->left_child->marker != 0) /* situation figure 17 */
    {
        case_hit = true;
    }
    else if (w->marker == w->parent->marker 
             && w->marker == w->right_child->marker 
             && w->marker != 0 
             && w->left_child->marker != 0) /* situation figure 18 */
    {
        case_hit = true;
    }
    else if (w->marker == w->parent->marker 
             && w->marker == w->right_child->marker 
             && w->marker != 0 
             && w->left_child->marker != 0) /* situation figure 19 */
    {
        case_hit = true;
    }

    if (!case_hit)
        return false;

    // build structure listing the nodes we hold flags on

    return true;
}

/**
 * @params
 *  z: target node (replace value)
 *  y: actually delete node
 */
bool setup_local_area_for_delete(tree_node *y, tree_node *z)
{
    tree_node *x;
    if (y->left_child->is_leaf)
        x = y->right_child;
    else
        x = y->left_child;
    
    // Try to get flags for the rest of the local area
    if (!x->flag.compare_exchange_weak(false, true)) return false; 
    
    tree_node *yp = y->parent; // keep a copy of our parent pointer
    if ((yp != z) && (!yp->flag.compare_exchange_weak(false, true)))
    {
        x->flag = false;
        return false;
    }
    if (yp != y->parent) // verify that parent is unchanged
    {  
        x->flag = false; 
        if (yp!=z) yp->flag = false;
        return false;
    }
    tree_node *w = y->parent->left_child;
    if (is_left(y))
        w = y->parent->right_child;
    
    if (!w->flag.compare_exchange_weak(false, true))
    {
        x->flag = false;
        if (yp != z)
            yp->flag = false;
        return false;
    }
    if (!w->is_leaf)
    {
        tree_node *wlc = w->left_child;
        tree_node *wrc = w->right_child;

        if (!wlc->flag.compare_exchange_weak(false, true))
        {
            x->flag = false;
            w->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
        if (!wrc->flag.compare_exchange_weak(false, true))
        {
            x->flag = false;
            w->flag = false;
            wlc->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
    }
    // TODO:
    // if (!GetFlagsAndMarkersAbove(yp, z))
    // {
    //     flag[x] = flag[w] = flag[wlc] = flag[wrc] = FALSE;
    //     if (yp != z)
    //         flag[yp] = FALSE;
    //     return false;
    // }
    return true;
}

/**
 * get flags for existing intention markers
 */
bool get_flags_for_markers(tree_node *start, move_up_struct *moveUpStruct,
                           tree_node **_pos1, tree_node **_pos2,
                           tree_node **_pos3, tree_node **_pos4)
{
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    if (!is_in(pos1, moveUpStruct) 
        && (!pos1->flag.compare_exchange_weak(false, true)))
        return false;
    if (pos1 != start->parent) // verify that parent is unchanged
    {  
        release_flag(moveUpStruct, false, pos1);
        return false;
    }
    pos2 = pos1->parent;
    if ((!is_in(pos2, moveUpStruct) 
        && !pos2->flag.compare_exchange_weak(false, true)))
    {
        pos1->flag = false;
        return false;
    }
    if (pos2 != pos1->parent) // verify that parent is unchanged
    {
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        return false;
    }
    pos3 = pos2->parent;
    if (!is_in(pos3, moveUpStruct) 
        && (!pos3->flag.compare_exchange_weak(false, true)))
    {
        pos1->flag = false;
        pos2->flag = false;
        return false;
    }
    if (pos3 != pos2->parent) // verify that parent is unchanged
    {
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        return false;
    }
    pos4 = pos3->parent;
    if (!is_in(pos4,moveUpStruct) 
        && (!pos4->flag.compare_exchange_weak(false, true)))
    {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        return false;
    }
    if (pos4 != pos3->parent) // verify that parent is unchanged
    {
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        release_flag(moveUpStruct, false, pos4);
        return false;
    }

    *_pos1 = pos1;
    *_pos2 = pos2;
    *_pos3 = pos3;
    *_pos4 = pos4;
    return true;
}

/**
 * add intention markers (four is sufficient) as needed
 */
bool get_flags_and_markers_above(tree_node *start, int numAdditional)
{
    /**
     * Check for a moveUpStruct provided by another process (due to 
     * Move-Up rule processing) and set ’PIDtoIgnore’ to the PID 
     * provided in that structure. Use the ’IsIn’ function to determine 
     * if a node is in the moveUpStruct. 
     * Start by getting flags on the four nodes we have markers on.
     */
    // TODO: need lock
    pthread_t self = pthread_self();
    move_up_struct *moveUpStruct = move_up_list[self];

    tree_node *pos1, *pos2, *pos3, *pos4;
    if (!get_flags_for_markers(start, moveUpStruct, &pos1, &pos2, &pos3, &pos4))
        return false;
    // Now get additional marker(s) above
    tree_node *firstnew = pos4->parent;
    if (!is_in(firstnew, moveUpStruct) 
        && (!firstnew->flag.compare_exchange_weak(false, true)))
    {
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        release_flag(moveUpStruct, false, pos4);
        return false;
    }
    // TODO: check this PID
    int PIDtoIgnore = moveUpStruct->other_close_process_PID;
    if ((firstnew != pos4->parent) 
        && (!spacing_rule_is_satisfied(firstnew, start, 
                                       PIDtoIgnore, moveUpStruct)))
    {
        release_flag(moveUpStruct, false, firstnew);
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        release_flag(moveUpStruct, false, pos4);
        return false;
    }

    tree_node *secondnew = NULL;
    if (numAdditional == 2) // insertion so need another marker
    {  
        secondnew = firstnew->parent;
        if ((!is_in(secondnew, moveUpStruct) 
            && !secondnew->flag.compare_exchange_weak(false, true)))
        {
            release_flag(moveUpStruct, false, firstnew);
            release_flag(moveUpStruct, false, pos1);
            release_flag(moveUpStruct, false, pos2);
            release_flag(moveUpStruct, false, pos3);
            release_flag(moveUpStruct, false, pos4);
            return false;
        }
        if ((secondnew != firstnew->parent) 
            && (!spacing_rule_is_satisfied(secondnew, start, 
                                           PIDtoIgnore, moveUpStruct)))
        {
            release_flag(moveUpStruct, false, secondnew);
            release_flag(moveUpStruct, false, firstnew);
            release_flag(moveUpStruct, false, pos1);
            release_flag(moveUpStruct, false, pos2);
            release_flag(moveUpStruct, false, pos3);
            release_flag(moveUpStruct, false, pos4);
            return false;
        }
    }

    pid_t my_pid = getpid();
    firstnew->marker = my_pid;
    if (numAdditional == 2)
        secondnew->marker = my_pid;

    // release the four topmost flags acquired to extend markers.
    // This leaves flags on nodes now in the new local area.
    if (numAdditional == 2)
        release_flag(moveUpStruct, true, secondnew);

    release_flag(moveUpStruct, true, firstnew);
    release_flag(moveUpStruct, true, pos4);
    release_flag(moveUpStruct, true, pos3);

    if (numAdditional == 1)
        release_flag(moveUpStruct, true, pos2);
    return true;
}

/**
 * TODO: ?
 */
bool setup_local_area_for_insert(tree_node *x)
{
    tree_node *parent = x->parent;
    tree_node *uncle = NULL;

    if (parent == NULL)
        return true;

    bool expected = false;
    if (!parent->flag.compare_exchange_weak(expected, true))
    {
        dbg_printf("[FLAG] failed getting flag of %lu\n", (unsigned long)parent);
        return false;
    }

    dbg_printf("[FLAG] get flag of %lu\n", (unsigned long)parent);

    // abort when parent of x changes
    if (parent != x->parent)
    {
        dbg_printf("[FLAG] parent changed from %lu to %lu\n", (unsigned long)parent, (unsigned long)parent);
        dbg_printf("[FLAG] release flag of %lu\n", (unsigned long)parent);
        parent->flag = false;
        return false;
    }

    if (is_left(x))
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

/**
 * TODO: what's local area here?
 */
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
    while (true && newx->parent != NULL)
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
        if (newgp == NULL)
            break;
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
 * return true if target node is in the moveUpStruct
 */
bool is_in(tree_node *target_node, move_up_struct *target_move_up_struct)
{
    if (!target_move_up_struct->valid)
    {
        return false;
    }
    
    if (target_move_up_struct->pos1 == target_node)
    {
        return true;
    }
    if (target_move_up_struct->pos2 == target_node)
    {
        return true;
    }
    if (target_move_up_struct->pos3 == target_node)
    {
        return true;
    }
    if (target_move_up_struct->pos4 == target_node)
    {
        return true;
    }

    return false;
}

/**
 * return true if target node is in the moveUpStruct
 */
bool is_goal(tree_node *target_node, move_up_struct *target_move_up_struct)
{
    if (!target_move_up_struct->valid)
    {
        return false;
    }

    if (target_move_up_struct->goal_node == target_node)
    {
        return true;
    }

    return false;
}
