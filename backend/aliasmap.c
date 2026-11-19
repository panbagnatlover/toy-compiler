/* Alias analysis.
 * Use discrete offset values.
 * Each pt set represent a different mem(base, offset).
 * 
 * Decision: instead of bitmap, use arena for mem state
 * objects. Use capped dynamic arrays for pt sets.
 * 
 * If a pt set has ALL_MEM, pt set = {ALL_MEM}.
 * 
 * Cap val() to 10 values. If above, widen to ALL_VAL.
 * 
 * Memories with different bases do not "overlap"
 * even if offset is ALL_VAL.
 * 
 * Widening operator: TBD, see example below
    int i = 0;
    while (cond) {
        arr[i] = i;
        i++;
    }
 * Analysis:
 * val(arr, [0]) = [0]
 * val(arr, [0, 1]) = [0, 1]
 * val(arr, [0, 2]) = [0, 2]
 * ...
 * val(arr, ALL_VAL) = ALL_VAL
 * 
 *  int i = 0;
    while (cond) {
        arr[i] = &x;
        i++;
    }
 * Analysis:
 * pt(arr, [0]) = {x}
 * pt(arr, [0, 1]) = {x}
 * ...
 * pt(arr, ALL_VAL) = {x}
 * 
 * Need to maintain mem(base, ALL_VAL) to be a union
 * of every mem(base, k). Same for val.
 * 
 * In the case of:
 * t = &x + b;
 * if (cond) goto L1
 * *t = 8 bytes
 * goto L2
 * L1: *t = 12 bytes
 * Memory can have both pt or val since we don't restrict
 * type at a memory.
 */

#include "arena.h"
#include "dfa.h"
#include "memory.h"

/* If val(x) can take on more than 10 constants,
 * set val(x) = ALL_VAL. */
#define VAL_K_THRES 12
#define MEM_K_THRES 10

typedef struct {
    ValType val_type;
    Array *val_set;  // Capped array of vals, 0 = NAI, 1 = ALL_VAL, 2-11 = other ints
} ValSet;

typedef struct {
    AliasType alias_type;
    void *pt_set;  // Capped array of mem state objs, sorted by base, or a single pointer
} PointToSet;

typedef struct {
    Memory mem;
    PointToSet pt_set;
    ValSet val_set;
} MemStateObj;

typedef struct {
    Arena *objs;
    /* TODO: some kind of data structure allowing for
     * sorted memory state object pointers. */
    Array *map;
} AliasState;

void i(void *) {};

/* Seed ERR_MEM and ALL_MEM. */
void seed_memories(AliasState *state) {
    MemStateObj err;
    err.mem.type = ERR_MEM;
    err.pt_set.alias_type = MUST_MEM;
    err.pt_set.pt_set = state->objs;  // Located at index 0 of arena

    MemStateObj all;
    all.mem.type = ALL_MEM;
    all.pt_set.alias_type = MAY_ALL_MEM;
    err.pt_set.pt_set = (char *) state->objs->head->arr + sizeof(MemStateObj);  // Located at index 1 of arena

    push_arena(&err, sizeof(MemStateObj), state->objs->head->arr);
    push_arena(&all, sizeof(MemStateObj), state->objs->head->arr);
}

MemStateObj *get_err_mem(AliasState *state) {
    return (MemStateObj *) state->objs->head->arr;
}

MemStateObj *get_all_mem(AliasState *state) {
    return &((MemStateObj *) state->objs->head->arr)[1];
}

MemStateObj *get_mem(Var *var, Val offset, AliasState *state) {
    MemStateObj **arr = (MemStateObj **) state->map->arr;
    size_t ind;  // TODO: 1 slot before insert location

    while (ind && arr[ind]->mem.base == var) {
        MemStateObj *obj = arr[ind--];
        if (obj->mem.base == var && eq_var(obj->mem.offset, offset))
            return obj;
    }
    return NULL;
}

MemStateObj *create_mem(Var *var, Val offset, AliasState *state) {
    MemStateObj obj;
    obj.mem.base = var;
    obj.mem.offset = offset;
    obj.mem.type = MEM;

    obj.pt_set.alias_type = NO_MEM;
    obj.pt_set.pt_set = NULL;

    obj.val_set.val_type = NO_VAL;
    obj.val_set.val_set = NULL;

    push_arena(&obj, sizeof(MemStateObj), state->objs);

    get loc in arena;

    push_array(&loc, state->map);  // TODO: in sorted order

    return loc;
}

// TODO: all bases sorted; offsets not sorted
/* a mem_type must be MAY_MEM. a->pt_set must be an array. */
bool insert_pt_set(PointToSet *a, MemStateObj *b) {
    if (a->alias_type != MAY_MEM) raise error;

    Array *pt_set = (Array *) a->pt_set;
    MemStateObj **arr = (MemStateObj **) pt_set->arr;

    bool changed = false;

    if (b->mem.type == ALL_MEM) {
        free_array(pt_set);
        a->pt_set = (void *) b;
        return true;
    }
    if (b->mem.type == ERR_MEM) {
        if (arr[0] != b) {
            changed = true;
            arr[0] = b;
        }
        return changed;
    }

    /* Widen offset values if reaching threshold. */
    uint8_t pt; // TODO - insertion point
    uint8_t ct;
    for (ct = 0; ct < VAL_K_THRES - 2; ct++) {
        if (arr[pt - ct - 1] == b) return false;
        if (arr[pt - ct - 1]->mem.base != b->mem.base) break;
        if (pt - ct == 1) break;
    }
    if (ct == VAL_K_THRES - 2) {
        // TODO - widen to mem(base, ALL_VAL);
        return true;
    }

    insert &b at the right loc;
    return true;
}

/* a->val_set must be an array. With widening. */
bool insert_val_set(ValSet *a, Val b) {
    Array *val_set = a->val_set;
    Val *arr = (Val *) val_set->arr;

    bool changed = false;

    if (b.type == NAI) {
        if (arr[0].type != NAI) {
            arr[0] = b;
            changed = true;
        }
        return changed;
    }
    if (b.type == ALL_VAL) {
        if (arr[1].type != ALL_VAL) {
            arr[1] = b;
            changed = true;
        }
        // TODO: update array size to collapse to 2
        return changed;
    }

    for (int i = 2; i < val_set->size; i++) {
        if (eq_val(b, arr[i])) return false;
    }

    if (val_set->size == VAL_K_THRES) {
        arr[1].type = ALL_VAL;
        // TODO: update array size to collapse to 2
        if (arr[0].type == NAI) a->val_type = MAY_ALL_NAI;
        else a->val_type = MAY_ALL_VAL;
        return true;
    }

    push_array(&b, val_set);
    return true;
}

bool union_pt_set(PointToSet *a, PointToSet *b) {
    Array *pt_set = (Array *) b->pt_set;
    MemStateObj **arr = (MemStateObj **) pt_set->arr;

    bool changed = false;

    for (int i = 0; i < pt_set->size; i++) {
        changed = changed || insert_pt_set(a, arr[i]);  
    }

    return changed;
}

bool union_val_set(ValSet *a, ValSet *b) {
    Array *val_set = (Array *) b->val_set;
    Val *arr = (Val *) val_set->arr;

    bool changed = false;

    for (int i = 0; i < val_set->size; i++) {
        changed = changed || insert_val_set(a, arr[i]);
    }

    return changed;
}

// TODO: assumes all bases are grouped together
/* Returns whether a pt set is widened to ALL_MEM. */
bool widen_pt_set(PointToSet *a, AliasState *state) {
    Array *pt_set = (Array *) a->pt_set;
    MemStateObj **arr = (MemStateObj **) pt_set->arr;

    Var *prev_base = NULL;
    uint8_t bases = 0;
    for (int i = 0; i < pt_set->size; i++) {
        if (prev_base != arr[i]) {
            bases++;
            prev_base = arr[i];

            /* Widen pt_set to ALL_MEM. */
            if (bases > MEM_K_THRES) {
                free_array(pt_set);
                a->pt_set = (void *) get_all_mem(state);
                return true;
            }
        }
    }
    return false;
}

bool meet_pt_set(MemStateObj *a, MemStateObj *b, AliasState *state) {
    AliasType a_type = a->pt_set.alias_type;
    AliasType b_type = b->pt_set.alias_type;
    void *a_pt = a->pt_set.pt_set;
    void *b_pt = b->pt_set.pt_set;

    bool changed = false;

    if (b_type == NO_MEM) { return false; }

    switch (a_type) {
        case NO_MEM:
            changed = true;
            a->pt_set.alias_type = b_type;
            if (b_type == MAY_MEM) {
                a->pt_set.pt_set = array(sizeof(MemStateObj *), i); // TODO - fixed array
                union_pt_set(&a->pt_set, &b->pt_set);
            }
            else a->pt_set.pt_set = b_pt;
            break;
        case MUST_MEM:
            switch (b_type) {
                case MUST_MEM:
                    if (a_pt == b_pt) break;
                    /* If pt(a) MUST= x and pt(b) MUST= y, 
                     * pt(a) MAY= {x, y} after meet. */
                    changed = true;
                    a->pt_set.alias_type = MAY_MEM;
                    a->pt_set.pt_set = array(sizeof(MemStateObj *), i); // TODO - fixed array
                    insert_pt_set(&a->pt_set, a_pt);
                    insert_pt_set(&a->pt_set, b_pt);
                    break;
                case MAY_MEM:
                    changed = true;
                    a->pt_set.alias_type = MAY_MEM;
                    a->pt_set.pt_set = array(sizeof(MemStateObj *), i); // TODO - fixed array
                    insert_pt_set(&a->pt_set, a_pt);
                    union_pt_set(&a->pt_set, &b->pt_set);
                    if (widen_pt_set(&a->pt_set, state))
                        a->pt_set.alias_type = MAY_ALL_MEM;
                    break;
                case MAY_ALL_MEM:
                    changed = true;
                    a->pt_set.alias_type = MAY_ALL_MEM;
                    a->pt_set.pt_set = get_all_mem(state);
                    break;
                default:
            }
            break;
        case MAY_MEM:
            switch (b_type) {
                case MUST_MEM:
                    changed = insert_pt_set(&a->pt_set, b_pt);
                    if (changed && widen_pt_set(&a->pt_set, state))
                        a->pt_set.alias_type = MAY_ALL_MEM;
                    break;
                case MAY_MEM:
                    changed = union_pt_set(&a->pt_set, &b->pt_set);
                    if (changed && widen_pt_set(&a->pt_set, state))
                        a->pt_set.alias_type = MAY_ALL_MEM;
                    break;
                case MAY_ALL_MEM:
                    changed = true;
                    a->pt_set.alias_type = MAY_ALL_MEM;
                    a->pt_set.pt_set = get_all_mem(state);
                    break;
                default:
            }
            break;
        case MAY_ALL_MEM:
            break;
        default:
    }

    /* Update summary, i.e., if pt(a, b) is updated,
     * need to update the pt set of (a, ALL_VAL). */
    if (a->pt_set.alias_type == MAY_MEM) {
        Val all; all.type = ALL_VAL;
        MemStateObj *summary = get_mem(a->mem.base, all, state);
        if (summary && a != summary) {
            changed = meet_pt_set(summary, a, state);
        }
    }

    return changed;
}

bool meet_val_set(MemStateObj *a, MemStateObj *b, AliasState *state) {
    ValType a_type = a->val_set.val_type;
    ValType b_type = b->val_set.val_type;
    void *a_vals = a->val_set.val_set;
    void *b_vals = b->val_set.val_set;

    bool changed = false;

    if (b_type == NO_VAL) { return false; }

    switch (a_type) {
        case NO_VAL:
            changed = true;
            a->val_set.val_type = b_type;
            a->val_set.val_set = array(sizeof(Val), i);  // TODO - fixed array, seed size 2
            union_val_set(&a->val_set, &b->val_set);
            break;
        case MUST_VAL:
            switch (b_type) {
                case MUST_VAL:
                    changed = union_val_set(&a->val_set, &b->val_set);
                    if (changed) a->val_set.val_type = MAY_VAL;
                    break;
                case MUST_NAI:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                default:
                    changed = true;
                    a->val_set.val_type = b_type;
                    union_val_set(&a->val_set, &b->val_set);
            }
            break;
        case MUST_NAI:
            switch (b_type) {
                case MUST_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MUST_NAI:
                    break;
                case MAY_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_NAI:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_ALL_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_ALL_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_ALL_NAI:
                    changed = true;
                    a->val_set.val_type = MAY_ALL_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                default:
            }
            break;
        case MAY_VAL:
            switch (b_type) {
                case MUST_VAL:
                    changed = true;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MUST_NAI:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_VAL:
                    changed = union_val_set(&a->val_set, &b->val_set);
                    break;
                default:
                    changed = true;
                    a->val_set.val_type = b_type;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
            }
            break;
        case MAY_NAI:
            switch (b_type) {
                case MUST_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MUST_NAI:
                    break;
                case MAY_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_NAI:
                    changed = union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_ALL_VAL:
                    changed = true;
                    a->val_set.val_type = MAY_ALL_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                case MAY_ALL_NAI:
                    changed = true;
                    a->val_set.val_type = MAY_ALL_NAI;
                    union_val_set(&a->val_set, &b->val_set);
                    break;
                default:
            }
            break;
        case MAY_ALL_VAL:
            if (b_type == MUST_NAI || b_type == MAY_NAI || b_type == MAY_ALL_NAI) {
                changed = true;
                a->val_set.val_type = MAY_ALL_NAI;
                union_val_set(&a->val_set, &b->val_set);
            }
            break;
        case MAY_ALL_NAI:
            break;
        default:
    }

    return changed;
}

void *init_state(void) {
    Arena *objs = arena(sizeof(MemStateObj), 256, i);
    Array *map = array(sizeof(Array *), free_array);  // TODO: initialize to VAR_POOL * 2
    AliasState *state = malloc(sizeof(AliasState));
    state->map = map;
    state->objs = objs;
    return (void *) state;
}

// x = y op z
bool tf_quad(Inst *inst, AliasState *state) {
    MemStateObj *x_obj = get_mem_state(inst->lhs, 0, state);
    if (y and z are constants) {
        if (y or z not integer) {
            union_alias(x_obj, MUST_ALIAS, array with err);
            union_val(x_obj, MUST_ALIAS, NAI_VAL);
        }
        else {
            union_alias(x_obj, MUST_ALIAS, array with err);
            union_val(x_obj, MUST_ALIAS, y op z);
        }
    }
    else if (y or z is constant) {
        c = whatever is constant;
        v = whatever is not constant;
        
        MemStateObj *v_obj = get_mem_state(v, 0, state);
        if (!v_obj) create_mem_state(v, 0, state);

        if (c MUST= NAI) {
            union_pt_set(x_obj, MUST_ALIAS, array with err);
        }
        else if (c MAY= NAI) {
            union_pt_set(x_obj, MAY_ALIAS, array with err);
        }

        if (v_obj->alias_type == MUST_ALIAS && err in v_obj->pt_set) {
            union_pt_set(x_obj, MUST_ALIAS, array with err);
        }
        else if (v_obj->alias_type == MUST_ALIAS) {
            union_pt_set(x_obj, MUST_ALIAS, v_obj->pt_set);
        }
        else if (all in pt set) {
            // TODO - pt(x) MAY ALIAS, pt(x) = ALL
        }
        // TODO - add condition for MAY err?
        else if (v_obj->alias_type == MAY_ALIAS) {
            union_pt_set(x_obj, MAY_ALIAS, v_obj->pt_set);
        }
    }
    else {
        if ()
    }

    // At the end, union with mem(x, ALL_VAL)
}

bool tf(void *b, void *s) {
    Block *block = (Block *) b;
    AliasState *state = (AliasState *) s;

    bool changed = false;

    for (inst in block->insts) {
        switch (inst->type) {
            case x = y:  // pt(x) = pt(y); val(x) = val(y)
                MemStateObj *x_obj = get_mem_state(x, 0, state);
                MemStateObj *y_obj = get_mem_state(y, 0, state);

                if (!x_obj) create_mem_state(x, 0, state);
                if (!y_obj) create_mem_state(y, 0, state);

                meet_pt_set(x_obj, y_obj, state);
                meet_val_set(x_obj, y_obj, state);

            /* TODO: frontend ensures y op z is valid. */
            case x = y op z:
                changed = tf_quad(inst, state);
            case *p = a:
                ...
            ...
        }
    }

    return changed;
}

AliasState *alias_analysis(Array *blocks) {
    void *state = dfa_generic(blocks, init_state, state_tf);
    return (AliasState *) state;
}
