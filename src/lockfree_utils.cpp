#include "tree.h"

#include <stdlib.h>
#include <pthread.h>
#include <vector>
#include <sys/types.h>

/******************
 * helper function
 ******************/

/* global variables */
move_up_struct *move_up_list = NULL;
pthread_mutex_t *move_up_lock_list = NULL;

/* thread-local variables */
thread_local vector<tree_node *> nodes_own_flag;
thread_local long lock_index;

/**
 * initialize the lock list for moveUpStruct list
 */
void move_up_lock_init(int num_processes)
{
    move_up_lock_list =
        (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * num_processes);
    for (int i = 0; i < num_processes; i++)
    {
        pthread_mutex_init(&move_up_lock_list[i], NULL);
    }
}

/**
 * initialize the moveUpStruct list for each thread
 */
void move_up_list_init(int num_processes)
{
    move_up_list = new move_up_struct[num_processes];
    for (int i = 0; i < num_processes; i++)
    {
        move_up_list[i].goal_node = NULL;
        move_up_list[i].nodes_with_flag.clear();
        move_up_list[i].other_close_process_TID = -1;
        move_up_list[i].valid = false;
    }
}

/**
 * cannot find a 'extern' way to use thread_local variables
 * across files
 */
void thread_lock_index_init(long i)
{
    lock_index = i;
}

/**
 * clear local area
 */
void clear_local_area(void)
{   
    if (nodes_own_flag.size() == 0) return;
    dbg_printf("[Flag] Clear\n");
    for (auto node : nodes_own_flag)
    {
        node->flag = false;
        dbg_printf("[Flag]      %d, 0x%lx, %d\n",
                   node->value, (unsigned long)node, (int)node->flag);
    }
    nodes_own_flag.clear();
}

/**
 * true if the node is in our local area
 */
bool is_in_local_area(tree_node *target_node)
{
    for (auto node : nodes_own_flag)
    {
        if (node == target_node) return true;        
    }
    
    return false;
}

/**
 * release flags unless node is in move up struct
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
                clear_self_moveUpStruct();
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
 * 
 * z - the node returned by par_find()
 */
bool spacing_rule_is_satisfied(tree_node *t, tree_node *z,
                                int PID_to_ignore,
                                move_up_struct *self_move_up_struct)
{
    bool expect;
    // We hold flags on both t and z.
    // check that t has no marker set
    if (t != z && t->marker != DEFAULT_MARKER && t->marker != PID_to_ignore) 
        return false;

    // check that t’s parent has no flag or marker
    tree_node *tp = t->parent;
    if (tp != z)
    {
        expect = false;
        if ((!is_in(tp, self_move_up_struct)) 
            && (!tp->flag.compare_exchange_weak(expect, true)))
        {
            return false;
        }
        if (tp != t->parent) // verify that parent is unchanged
        {  
            tp->flag = false;
            return false;
        }
        if (tp->marker != DEFAULT_MARKER && tp->marker != PID_to_ignore)
        {
            tp->flag = false;
            return false;
        }
    }
    // check that t’s sibling has no flag or marker
    tree_node *ts = tp->left_child;
    if (is_left(t))
        ts = tp->right_child;
    
    expect = false;
    if ((!is_in(ts, self_move_up_struct)) 
        && (!ts->flag.compare_exchange_weak(expect, true)))
    {
        if (tp != z)
            release_flag(self_move_up_struct, false, tp);
        return false;
    }
    if ((ts->marker != DEFAULT_MARKER) && (ts->marker != PID_to_ignore))
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
        && w->marker != DEFAULT_MARKER 
        && w->left_child->marker != DEFAULT_MARKER) /* case figure 17 */
    {
        case_hit = true;
    }
    else if (w->marker == w->parent->marker 
             && w->marker == w->right_child->marker 
             && w->marker != DEFAULT_MARKER 
             && w->left_child->marker != DEFAULT_MARKER) /* case figure 18 */
    {
        case_hit = true;
    }
    else if (w->marker == w->parent->marker 
             && w->marker == w->right_child->marker 
             && w->marker != DEFAULT_MARKER 
             && w->left_child->marker != DEFAULT_MARKER) /* case figure 19 */
    {
        case_hit = true;
    }

    if (!case_hit)
        return false;

    dbg_printf("[Remove] Apply move up rule.\n");
    // Claim that struct of the inherit one is not valid at this moment
    // Otherwise, every process will have more than one valid struct 
    // at the same time
    int tid_left = w->left_child->marker;
    int tid_right = w->right_child->marker;

    int move_up_tid = tid_right;
    int other_tid = tid_left;
    if (w->marker == tid_left)
    {
        move_up_tid = tid_left;
        other_tid = tid_right;
    }
    
    // TODO: check this lock area
    pthread_mutex_lock(&move_up_lock_list[move_up_tid]);
    move_up_struct *moveUpStruct = &move_up_list[move_up_tid];
    moveUpStruct->other_close_process_TID = other_tid;
    moveUpStruct->goal_node = w->parent->parent;
    moveUpStruct->nodes_with_flag.clear();
    for (auto node : nodes_own_flag)
        moveUpStruct->nodes_with_flag.push_back(node);
    moveUpStruct->valid = true;
    pthread_mutex_unlock(&move_up_lock_list[move_up_tid]);

    return true;
}

/**
 * clear self moveUpStruct if possible after deletion done
 */
void clear_self_moveUpStruct(void)
{
    pthread_mutex_lock(&move_up_lock_list[lock_index]);
    move_up_struct *moveUpStruct = &move_up_list[lock_index];
    if (!moveUpStruct->valid) 
    {
        pthread_mutex_unlock(&move_up_lock_list[lock_index]);
        return;
    }
    for (auto node : moveUpStruct->nodes_with_flag)
        node->flag = false;
    moveUpStruct->nodes_with_flag.clear();
    moveUpStruct->valid = false;
    pthread_mutex_unlock(&move_up_lock_list[lock_index]);
}

/**
 * try to get four markers above
 * 
 * @params:
 *      start - parent node of the actual node to be deleted
 */
bool get_markers_above(tree_node *start, tree_node *z, bool release)
{
    move_up_struct *moveUpStruct = &move_up_list[lock_index];
    bool expect;

    // Now get marker(s) above
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    if (pos1 != z)
    {
        expect = false;
        if (!is_in(pos1, moveUpStruct) 
            && (!pos1->flag.compare_exchange_weak(expect, true)))
            return false;
    }
    
    if ((pos1 != start->parent) 
        || (!spacing_rule_is_satisfied(pos1, z,
                                       lock_index, moveUpStruct)))
    {
        if (pos1 != z) 
            release_flag(moveUpStruct, false, pos1);
        return false;
    }

    pos2 = pos1->parent;
    if (pos2 != z)
    {
        expect = false;
        if (!is_in(pos2, moveUpStruct) 
            && (!pos2->flag.compare_exchange_weak(expect, true)))
        {
            if (pos1 != z)
                release_flag(moveUpStruct, false, pos1);
            return false;
        }
    }

    if ((pos2 != pos1->parent) 
        || (!spacing_rule_is_satisfied(pos2, z,
                                       lock_index, moveUpStruct)))
    {
        if (pos1 != z)
            release_flag(moveUpStruct, false, pos1);
        if (pos2 != z)
            release_flag(moveUpStruct, false, pos2);
        return false;
    }

    pos3 = pos2->parent;
    if (pos3 != z)
    {
        expect = false;
        if (!is_in(pos3, moveUpStruct) 
            && (!pos3->flag.compare_exchange_weak(expect, true)))
        {
            if (pos1 != z)
                release_flag(moveUpStruct, false, pos1);
            if (pos2 != z)
                release_flag(moveUpStruct, false, pos2);
            return false;
        }
    }

    if ((pos3 != pos2->parent) 
        || (!spacing_rule_is_satisfied(pos3, z,
                                       lock_index, moveUpStruct)))
    {
        if (pos1 != z)
            release_flag(moveUpStruct, false, pos1);
        if (pos2 != z)
            release_flag(moveUpStruct, false, pos2);
        if (pos3 != z)
            release_flag(moveUpStruct, false, pos3);
        return false;
    }

    pos4 = pos3->parent;
    if (pos4 != z)
    {
        expect = false;
        if (!is_in(pos4, moveUpStruct) 
            && (!pos4->flag.compare_exchange_weak(expect, true)))
        {
            if (pos1 != z)
                release_flag(moveUpStruct, false, pos1);
            if (pos2 != z)
                release_flag(moveUpStruct, false, pos2);
            if (pos3 != z)
                release_flag(moveUpStruct, false, pos3);
            return false;
        }
    }
    
    if ((pos4 != pos3->parent) 
        || (!spacing_rule_is_satisfied(pos4, z,
                                       lock_index, moveUpStruct)))
    {
        if (pos1 != z)
            release_flag(moveUpStruct, false, pos1);
        if (pos2 != z)
            release_flag(moveUpStruct, false, pos2);
        if (pos3 != z)
            release_flag(moveUpStruct, false, pos3);
        if (pos4 != z)
            release_flag(moveUpStruct, false, pos4);
        return false;
    }

    // successfully get the four markers
    pos1->marker = lock_index;
    pos2->marker = lock_index;
    pos3->marker = lock_index;
    pos4->marker = lock_index;

    if (release)
    {
        if (pos1 != z)
            release_flag(moveUpStruct, false, pos1);
        if (pos2 != z)
            release_flag(moveUpStruct, false, pos2);
        if (pos3 != z)
            release_flag(moveUpStruct, false, pos3);
        if (pos4 != z)
            release_flag(moveUpStruct, false, pos4);
    }

    dbg_printf("[Flag] get for marker: %d %d %d %d\n",
               pos1->value, pos2->value, pos3->value, pos4->value);

    return true;
}

/**
 * @params
 *  z: target node (replace value)
 *  y: actually delete node
 */
bool setup_local_area_for_delete(tree_node *y, tree_node *z)
{
    bool expect;
    // the replace child, the actual target node
    tree_node *x = y->left_child;
    if (y->left_child->is_leaf)
        x = y->right_child;
    
    expect = false;
    // Try to get flags for the rest of the local area
    if (!x->flag.compare_exchange_weak(expect, true)) return false;
    
    tree_node *yp = y->parent; // keep a copy of our parent pointer
    expect = false;
    if ((yp != z) && (!yp->flag.compare_exchange_weak(expect, true)))
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
    
    expect = false;
    if (!w->flag.compare_exchange_weak(expect, true))
    {
        x->flag = false;
        if (yp != z)
            yp->flag = false;
        return false;
    }

    tree_node *wlc, *wrc;
    if (!w->is_leaf)
    {
        wlc = w->left_child;
        wrc = w->right_child;

        expect = false;
        if (!wlc->flag.compare_exchange_weak(expect, true))
        {
            x->flag = false;
            w->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
        expect = false;
        if (!wrc->flag.compare_exchange_weak(expect, true))
        {
            x->flag = false;
            w->flag = false;
            wlc->flag = false;
            if (yp != z)
                yp->flag = false;
            return false;
        }
    }

    if (!get_markers_above(yp, z, true))
    {
        x->flag = false;
        w->flag = false;
        if (!w->is_leaf)
        {
            wlc->flag = false;
            wrc->flag = false;
        }
        if (yp != z)
            yp->flag = false;
        return false;
    }

    // local area setup
    nodes_own_flag.push_back(x);
    nodes_own_flag.push_back(w);
    nodes_own_flag.push_back(yp);
    if (!w->is_leaf)
    {
        nodes_own_flag.push_back(wlc);
        nodes_own_flag.push_back(wrc);
        dbg_printf("[Flag] local area: %d %d %d %d %d\n",
                   x->value, w->value, yp->value, wlc->value, wrc->value);
    }
    else
    {
        dbg_printf("[Flag] local area: %d %d %d\n",
                   x->value, w->value, yp->value);
    }

    return true;
}

/**
 * get flags for existing intention markers
 */
bool get_flags_for_markers(tree_node *start, move_up_struct *moveUpStruct,
                           tree_node **_pos1, tree_node **_pos2,
                           tree_node **_pos3, tree_node **_pos4)
{
    bool expect;
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    expect = false;
    if (!is_in(pos1, moveUpStruct) 
        && (!pos1->flag.compare_exchange_weak(expect, true)))
        return false;
    if (pos1 != start->parent) // verify that parent is unchanged
    {  
        release_flag(moveUpStruct, false, pos1);
        return false;
    }
    pos2 = pos1->parent;
    expect = false;
    if ((!is_in(pos2, moveUpStruct) 
        && !pos2->flag.compare_exchange_weak(expect, true)))
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
    expect = false;
    if (!is_in(pos3, moveUpStruct) 
        && (!pos3->flag.compare_exchange_weak(expect, true)))
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
    expect = false;
    if (!is_in(pos4,moveUpStruct) 
        && (!pos4->flag.compare_exchange_weak(expect, true)))
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

    // get markers first and do not release flag
    if (!get_markers_above(start, NULL, false))
        return false;

    move_up_struct *moveUpStruct = &move_up_list[lock_index];
    bool expect;
    tree_node *pos1 = start->parent;
    tree_node *pos2 = pos1->parent;
    tree_node *pos3 = pos2->parent;
    tree_node *pos4 = pos3->parent;
    // no need of this function
    // if (!get_flags_for_markers(start, moveUpStruct, &pos1, &pos2, &pos3, &pos4))
    //     return false;

    // Now get additional marker(s) above
    tree_node *firstnew = pos4->parent;
    expect = false;
    if (!is_in(firstnew, moveUpStruct) 
        && (!firstnew->flag.compare_exchange_weak(expect, true)))
    {
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        release_flag(moveUpStruct, false, pos4);
        return false;
    }

    // avoid the other close PID when chosen to move up
    int PIDtoIgnore = moveUpStruct->other_close_process_TID;
    if ((firstnew != pos4->parent) 
        || (!spacing_rule_is_satisfied(firstnew, start, 
                                       PIDtoIgnore, moveUpStruct)))
    {
        release_flag(moveUpStruct, false, firstnew);
        release_flag(moveUpStruct, false, pos1);
        release_flag(moveUpStruct, false, pos2);
        release_flag(moveUpStruct, false, pos3);
        release_flag(moveUpStruct, false, pos4);
        return false;
    }

    dbg_printf("[Flag] firstnew: %d\n",
               firstnew->value);

    tree_node *secondnew = NULL;
    if (numAdditional == 2) // insertion so need another marker
    {  
        secondnew = firstnew->parent;
        expect = false;
        if ((!is_in(secondnew, moveUpStruct) 
            && !secondnew->flag.compare_exchange_weak(expect, true)))
        {
            release_flag(moveUpStruct, false, firstnew);
            release_flag(moveUpStruct, false, pos1);
            release_flag(moveUpStruct, false, pos2);
            release_flag(moveUpStruct, false, pos3);
            release_flag(moveUpStruct, false, pos4);
            return false;
        }

        if ((secondnew != firstnew->parent) 
            || (!spacing_rule_is_satisfied(secondnew, start, 
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
        dbg_printf("[Flag] second new: %d\n",
                   secondnew->value);
    }

    firstnew->marker = lock_index;
    if (numAdditional == 2)
        secondnew->marker = lock_index;

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
 * try to release four markers above
 * this is the anti-function of get_markers_above()
 * this should always be valid if the rotation fixups are doing well
 * 
 * @params:
 *      start - parent node of the actual node to be deleted
 *      z - the node returned by par_find()
 */
bool release_markers_above(tree_node *start, tree_node *z)
{
    bool expect;
    
    // release 4 marker(s) above start node
    tree_node *pos1, *pos2, *pos3, *pos4;

    pos1 = start->parent;
    expect = false;
    if (!pos1->flag.compare_exchange_weak(expect, true))
        return false;
    if (pos1 != start->parent) // verify that parent is unchanged
    {  
        pos1->flag = false;
        return false;
    }
    pos2 = pos1->parent;
    expect = false;
    if (!pos2->flag.compare_exchange_weak(expect, true))
    {
        pos1->flag = false;
        return false;
    }
    if (pos2 != pos1->parent) // verify that parent is unchanged
    {
        pos1->flag = false;
        pos2->flag = false;
        return false;
    }
    pos3 = pos2->parent;
    expect = false;
    if (!pos3->flag.compare_exchange_weak(expect, true))
    {
        pos1->flag = false;
        pos2->flag = false;
        return false;
    }
    if (pos3 != pos2->parent) // verify that parent is unchanged
    {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        return false;
    }
    pos4 = pos3->parent;
    expect = false;
    if (!pos4->flag.compare_exchange_weak(expect, true))
    {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        return false;
    }
    if (pos4 != pos3->parent) // verify that parent is unchanged
    {
        pos1->flag = false;
        pos2->flag = false;
        pos3->flag = false;
        pos4->flag = false;
        return false;
    }

    // release these markers
    if (pos1->marker == lock_index) pos1->marker = DEFAULT_MARKER;
    if (pos2->marker == lock_index) pos2->marker = DEFAULT_MARKER;
    if (pos3->marker == lock_index) pos3->marker = DEFAULT_MARKER;
    if (pos4->marker == lock_index) pos4->marker = DEFAULT_MARKER;

    dbg_printf("[Marker] release markers %d %d %d %d\n",
                pos1->value, pos2->value, pos3->value, pos4->value);
    
    // release flags, TODO: moveUpStruct?
    pos1->flag = false;
    pos2->flag = false;
    pos3->flag = false;
    pos4->flag = false;

    return true;
}

/**
 * move a deleter up the tree
 * case 2 in deletion
 */
tree_node *move_deleter_up(tree_node *oldx)
{
    // here is the only place to lock the struct
    pthread_mutex_lock(&move_up_lock_list[lock_index]);
    move_up_struct *moveUpStruct = &move_up_list[lock_index];
    bool expect;

    // TODO: check for a moveUpStruct from another process
    // get direct pointers
    tree_node *oldp = oldx->parent;
    tree_node *oldw = oldp->left_child;
    if (is_left(oldx))
        oldw = oldp->right_child;

    tree_node *oldwlc = oldw->left_child;
    tree_node *oldwrc = oldw->right_child;

    // extend intention markers (getting flags to set them)
    // from oldgp to top and one more. Also convert marker on oldgp to a flag
    while (!get_flags_and_markers_above(oldp, 1))
        ;
    
    // get flags on the rest of new local area (w, wlc, wrc)
    tree_node *newx = oldp;
    tree_node *newp = newx->parent;
    tree_node *neww = newp->left_child;
    if (is_left(newx))
        neww = newp->right_child;
    
    if (!is_in(neww, moveUpStruct))
    {
        do {
            expect = false;
        } while (!neww->flag.compare_exchange_weak(expect, true));
    }
    tree_node *newwlc = neww->left_child;
    tree_node *newwrc = neww->right_child;
    if (!is_in(newwlc, moveUpStruct))
    {
        do {
            expect = false;
        } while (!newwlc->flag.compare_exchange_weak(expect, true));
    }
    if (!is_in(newwrc, moveUpStruct))
    {
        do {
            expect = false;
        } while (!newwrc->flag.compare_exchange_weak(expect, true));
    }

    // release flags on old local area
    release_flag(moveUpStruct, true, oldx);
    release_flag(moveUpStruct, true, oldw);
    release_flag(moveUpStruct, true, oldwlc);
    release_flag(moveUpStruct, true, oldwrc);

    dbg_printf("[Flag] release old local area: %d %d %d %d\n",
                oldx->value, oldw->value, oldwlc->value, oldwrc->value);

    pthread_mutex_unlock(&move_up_lock_list[lock_index]);

    // clear marker
    newx->parent->marker = DEFAULT_MARKER;

    // new local area
    nodes_own_flag.clear();
    nodes_own_flag.push_back(newx);
    nodes_own_flag.push_back(neww);
    nodes_own_flag.push_back(newp);
    nodes_own_flag.push_back(newwlc);
    nodes_own_flag.push_back(newwrc);
    dbg_printf("[Flag] get new local area: %d %d %d %d %d\n",
               newx->value, neww->value, newp->value, 
               newwlc->value, newwrc->value);
    return newx;
}


/**
 * fix the side effect of delete case 1
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation 
 *    --> clear all the 'naughty' markers within old local area
 */
void fix_up_case1(tree_node *x, tree_node *w)
{
    tree_node *oldw = x->parent->parent;
    tree_node *oldwlc = x->parent->right_child;
    tree_node *oldwrc = oldw->right_child;

    // clear markers
    if (oldw->marker != DEFAULT_MARKER && oldw->marker == oldwlc->marker)
    {
        x->parent->marker = oldw->marker;
    }

    // set w's marker before releasing its flag
    oldw->marker = lock_index;
    oldw->flag = false;
    oldwrc->flag = false;
    dbg_printf("[Flag] release %d %d\n", oldw->value, oldwrc->value);

    // release the fifth marker
    oldw->parent->parent->parent->parent->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // TODO: this will always be valid because of Spacing Rule
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    w->right_child->flag = true;
    dbg_printf("[Flag] get new %d %d\n", 
                w->left_child->value, w->right_child->value);

    // new local area
    nodes_own_flag.clear();
    nodes_own_flag.push_back(x);
    nodes_own_flag.push_back(x->parent);
    nodes_own_flag.push_back(w);
    nodes_own_flag.push_back(w->left_child);
    nodes_own_flag.push_back(w->right_child);
}

/**
 * fix the side effect of delete case 3
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation
 * layout:
 *    2
 * 1     3
 *     4   5
 *    6 7
 */
void fix_up_case3(tree_node *x, tree_node *w)
{
    tree_node *oldw = w->right_child;
    tree_node *oldwrc = oldw->right_child;

    // clear all the markers within old local area
    for (auto node : nodes_own_flag)
        node->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // TODO: this will always be valid because of Spacing Rule
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    oldwrc->flag = false;
    dbg_printf("[Flag] release %d, get %d\n", 
                oldwrc->value, w->left_child->value);

    // new local area
    nodes_own_flag.clear();
    nodes_own_flag.push_back(x);
    nodes_own_flag.push_back(x->parent);
    nodes_own_flag.push_back(w);
    nodes_own_flag.push_back(w->left_child);
    nodes_own_flag.push_back(oldw);
}

/**
 * fix the side effect of delete case 1 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation 
 *    --> clear all the 'naughty' markers within old local area
 */
void fix_up_case1_r(tree_node *x, tree_node *w)
{
    tree_node *oldw = x->parent->parent;
    tree_node *oldwlc = oldw->left_child;
    tree_node *oldwrc = x->parent->left_child;

    // clear markers
    if (oldw->marker != DEFAULT_MARKER && oldw->marker == oldwrc->marker)
    {
        x->parent->marker = oldw->marker;
    }

    // set w's marker before releasing its flag
    oldw->marker = lock_index;
    oldw->flag = false;
    oldwlc->flag = false;
    dbg_printf("[Flag] release %d %d\n",
               oldw->value, oldwlc->value);
    // release the fifth marker
    oldw->parent->parent->parent->parent->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // TODO: this will always be valid because of Spacing Rule
    // which means others may hold markers on them, but no flags on them
    w->left_child->flag = true;
    w->right_child->flag = true;
    dbg_printf("[Flag] get new %d %d\n",
               w->left_child->value, w->right_child->value);
    // new local area
    nodes_own_flag.clear();
    nodes_own_flag.push_back(x);
    nodes_own_flag.push_back(x->parent);
    nodes_own_flag.push_back(w);
    nodes_own_flag.push_back(w->left_child);
    nodes_own_flag.push_back(w->right_child);
}

/**
 * fix the side effect of delete case 3 - mirror case
 * 1. adjust the local area of the process that has done the rotation
 * 2. move any relocated markers from other processes to where 
 *    they belong after the rotation
 */
void fix_up_case3_r(tree_node *x, tree_node *w)
{
    tree_node *oldw = w->left_child;
    tree_node *oldwlc = oldw->left_child;

    // clear all the markers within old local area
    for (auto node : nodes_own_flag)
        node->marker = DEFAULT_MARKER;

    // get the flag of the new wlc & wrc
    // TODO: this will always be valid because of Spacing Rule
    // which means others may hold markers on them, but no flags on them
    w->right_child->flag = true;
    oldwlc->flag = false;
    dbg_printf("[Flag] release %d, get %d\n",
               oldwlc->value, w->right_child->value);
    // new local area
    nodes_own_flag.clear();
    nodes_own_flag.push_back(x);
    nodes_own_flag.push_back(x->parent);
    nodes_own_flag.push_back(w);
    nodes_own_flag.push_back(oldw);
    nodes_own_flag.push_back(w->right_child);
}

/************************ insert ************************/
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
        dbg_printf("[FLAG] failed getting flag of 0x%lx\n", 
                   (unsigned long)parent);
        return false;
    }

    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)parent);

    // abort when parent of x changes
    if (parent != x->parent)
    {
        dbg_printf("[FLAG] parent changed from %lu to 0x%lx\n", 
                   (unsigned long)parent, (unsigned long)parent);
        dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)parent);
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
        dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)uncle);
        dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)x->parent);
        x->parent->flag = false;
        return false;
    }

    dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)uncle);

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
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newp);
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newp);

        newgp = newp->parent;
        if (newgp == NULL)
            break;
        expected = false;
        if (!newgp->flag.compare_exchange_weak(expected, true))
        {
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newp);
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newgp);

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
            dbg_printf("[FLAG] failed getting flag of 0x%lx\n", (unsigned long)newuncle);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newgp);
            dbg_printf("[FLAG] release flag of 0x%lx\n", (unsigned long)newp);
            newgp->flag = false;
            newp->flag = false;
            continue;
        }

        dbg_printf("[FLAG] get flag of 0x%lx\n", (unsigned long)newuncle);

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
    
    for (auto node : target_move_up_struct->nodes_with_flag)
    {
        if (target_node == node) return true;
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

/**
 * find a node by getting flag hand over hand
 * restart when conflict happens
 */
tree_node *par_find(tree_node *root, int value)
{
    bool expect;
restart:
    tree_node *root_node;
    do
    {
        root_node = root->left_child;
        expect = false;
    } while (!root_node->flag.compare_exchange_weak(expect, true));
    
    tree_node *y = root_node;
    tree_node *z = NULL;

    while (!y->is_leaf)
    {
        z = y; // store old y
        if (value == y->value)
            return y; // find the node y
        else if (value > y->value)
            y = y->right_child;
        else
            y = y->left_child;
        
        expect = false;
        if (!y->flag.compare_exchange_weak(expect, true))
        {
            z->flag = false; // release held flag
            usleep(100);
            goto restart;
        }
        if (!y->is_leaf)
            z->flag = false; // release old y's flag
    }
    
    dbg_printf("[WARNING] node with value %d not found.\n", value);
    return NULL; // node not found
}

/**
 * find a node's successor on the left
 * already make sure that the delete node have two non-leaf children
 * by getting flag hand over hand
 * restart when conflict happens
 */
tree_node *par_find_successor(tree_node *delete_node)
{
    bool expect;
    // we already hold the flag of delete_node

    tree_node *y = delete_node->right_child;
    tree_node *z = NULL;

    while (!y->left_child->is_leaf)
    {
        z = y; // store old y
        y = y->left_child;

        expect = false;
        if (!y->flag.compare_exchange_weak(expect, true))
        {
            z->flag = false; // release held flag
            return NULL; // restart outside
        }
        
        z->flag = false; // release old y's flag
    }
    
    return y; // successor found
}
