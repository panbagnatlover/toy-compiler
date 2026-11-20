/* Alias analysis.
 * Use discrete offset values.
 * Each pt set represent a different mem(base, offset).
 * 
 * Decision: instead of bitmap, use arena for mem state
 * objects. Use capped dynamic arrays for pt sets.
 * All raw mem address (e.g., x = 0x12345) is removed
 * from 3ac. x is treated as a global var located at 0x12345,
 * so does not need to account for this situation.
 * Similarly, cannot have x = NULL + y, etc.
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
 * goto L2
 * L1: *t = 12 bytes
 * Memory can have both pt or val since we don't restrict
 * type at a memory.
 * 
 * TODO:
 * - insert_pt_set and insert_val_set, vs _op_val and _op_mem:
 * param is copy or reference?
 * - Free temp arrays correctly
 * - custom allocation for mem and val arrays with garbage collection?
 */

#include "arena.h"
#include "dfa.h"
#include "memory.h"
#include "inst.h"

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

void *init_state(void) {
    Arena *objs = arena(sizeof(MemStateObj), 256, i);
    Array *map = array(sizeof(Array *), free_array);  // TODO: initialize to VAR_POOL * 2
    AliasState *state = malloc(sizeof(AliasState));
    state->map = map;
    state->objs = objs;
    return (void *) state;
}

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

// TODO: need to free temp array size if needed.

/* Transfer function for x = y: pt(x) = pt(x) meet pt(y). */
bool tf_copy(Inst *inst, AliasState *state) {
    bool changed = false;

    Operand lhs = inst->lhs;
    Operand rhs = inst->rhs;

    /* Get LHS's memory object. */
    Val offset; offset.type = VAL; offset.value = 0;
    MemStateObj *l = get_mem(lhs.var, offset, state);
    if (!l) l = create_mem(lhs.var, offset, state);

    if (rhs.type == CONST) {
        /* Create a temp memory object. */
        MemStateObj t;
        t.pt_set.alias_type = MUST_MEM;
        t.pt_set.pt_set = get_err_mem(state);

        if (rhs.constant.type == INT) {
            t.val_set.val_type = MUST_VAL;
            t.val_set.val_set = array containing rhs.constant.value;
        }
        else {
            t.val_set.val_type = MUST_NAI;
            t.val_set.val_set = array containing NAI;
        }

        changed = changed || meet_pt_set(l, &t, state);
        changed = changed || meet_val_set(l, &t, state);
    }
    else {
        /* Get RHS's memory object. */
        MemStateObj *r = get_mem(rhs.var, offset, state);
        if (!r) r = create_mem(rhs.var, offset, state);

        changed = changed || meet_pt_set(l, r, state);
        changed = changed || meet_val_set(l, r, state);
    }

    return changed;
}

/* Transfer function for x = &y: pt(x) = pt(x) meet MUST {x}. */
bool tf_addr(Inst *inst, AliasState *state) {
    bool changed = false;

    Operand lhs = inst->lhs;
    Operand rhs = inst->rhs;

    /* Get LHS's memory object. */
    Val offset; offset.type = VAL; offset.value = 0;
    MemStateObj *l = get_mem(lhs.var, offset, state);
    if (!l) l = create_mem(lhs.var, offset, state);

    /* Get RHS's memory object. */
    MemStateObj *r = get_mem(rhs.var, offset, state);
    if (!r) r = create_mem(rhs.var, offset, state);

    /* Create a temp memory object with MUST {x} pt set
     * and MUST_NAI val set. */
    MemStateObj t;
    t.pt_set.alias_type = MUST_MEM;
    t.pt_set.pt_set = array containing r;
    t.val_set.val_type = MUST_NAI;
    t.val_set.val_set = array containing NAI;

    changed = changed || meet_pt_set(l, &t, state);
    changed = changed || meet_val_set(l, &t, state);

    return changed;
}

/* Transfer function for x = *y:
 * If pt(y) MUST= q, pt(q) MUST= p, pt(x) = pt(x) meet MUST {p}.
 * If pt(y) MAY= q, pt(q) MUST= p, pt(x) = pt(x) meet MAY {p}.
 * Else, pt(x) = pt(x) meet pt(q) for all q in pt(y). */
bool tf_rhs_deref(Inst *inst, AliasState *state) {
    bool changed = false;

    Operand lhs = inst->lhs;
    Operand rhs = inst->rhs;

    /* Get LHS's memory object. */
    Val offset; offset.type = VAL; offset.value = 0;
    MemStateObj *l = get_mem(lhs.var, offset, state);
    if (!l) l = create_mem(lhs.var, offset, state);

    /* Get RHS's memory object. */
    MemStateObj *r = get_mem(rhs.var, offset, state);
    if (!r) r = create_mem(rhs.var, offset, state);

    if (r->pt_set.alias_type == MUST_MEM || r->pt_set.alias_type == MAY_ALL_MEM) {
        MemStateObj *q = (MemStateObj *) r->pt_set.pt_set;
        changed = changed || meet_pt_set(l, q, state);
        changed = changed || val_pt_set(l, q, state);
    }
    else {
        Array *pt_set = (Array *) r->pt_set.pt_set;
        MemStateObj **arr = (MemStateObj **) pt_set->arr;

        for (int i = 0; i < pt_set->size; i++) {
            changed = changed || meet_pt_set(l, arr[i], state);
            changed = changed || val_pt_set(l, arr[i], state);
        }

        /* pt(LHS) cannot have a MUST relationship if
         * pt(RHS) has a MAY relationship. Same for Val(). */
        if (l->pt_set.alias_type == MUST_MEM) {
            changed = true;
            l->pt_set.alias_type = MAY_MEM;
        }
        if (l->val_set.val_type == MUST_VAL) {
            changed = true;
            l->val_set.val_type = MAY_VAL;
        }
        if (l->val_set.val_type == MAY_NAI) {
            changed = true;
            l->val_set.val_type = MAY_NAI;
        }
    }

    return changed;
}

/* Transfer function for *x = y:
 * If pt(x) MUST= q, pt(y) MUST= p, pt(q) = pt(q) meet MUST {p}.
 * If pt(x) MAY= q, pt(y) MUST= p, pt(q) = pt(q) meet MAY {p}.
 * Else, pt(q) = pt(q) meet pt(y) for all q in pt(x). */
bool tf_lhs_deref(Inst *inst, AliasState *state) {
    bool changed = false;

    Operand lhs = inst->lhs;
    Operand rhs = inst->rhs;

    /* Get LHS's memory object. */
    Val offset; offset.type = VAL; offset.value = 0;
    MemStateObj *l = get_mem(lhs.var, offset, state);
    if (!l) l = create_mem(lhs.var, offset, state);

    /* Get RHS's memory object. */
    MemStateObj *r = get_mem(rhs.var, offset, state);
    if (!r) r = create_mem(rhs.var, offset, state);

    if (l->pt_set.alias_type == MUST_MEM || l->pt_set.alias_type == MAY_ALL_MEM) {
        MemStateObj *q = (MemStateObj *) l->pt_set.pt_set;
        changed = changed || meet_pt_set(q, r, state);
        changed = changed || val_pt_set(q, r, state);
    }
    else {
        Array *pt_set = (Array *) l->pt_set.pt_set;
        MemStateObj **arr = (MemStateObj **) pt_set->arr;

        for (int i = 0; i < pt_set->size; i++) {
            changed = changed || meet_pt_set(arr[i], r, state);
            changed = changed || val_pt_set(arr[i], r, state);

            /* pt(LHS) cannot have a MUST relationship if
            * pt(RHS) has a MAY relationship. Same for val(). */
            if (arr[i]->pt_set.alias_type == MUST_MEM) {
                changed = true;
                arr[i]->pt_set.alias_type = MAY_MEM;
            }
            if (arr[i]->val_set.val_type == MUST_VAL) {
                changed = true;
                arr[i]->val_set.val_type = MAY_VAL;
            }
            if (arr[i]->val_set.val_type == MAY_NAI) {
                changed = true;
                arr[i]->val_set.val_type = MAY_NAI;
            }
        }
    }

    return changed;
}

/* Assumes a is a mem and b is an offset. */
PointToSet _op_mem(PointToSet a, ValSet b, Op op, AliasState *state) {
    PointToSet ret;

    if (a.alias_type == NO_MEM || b.val_type == NO_VAL) {
        ret.alias_type = NO_MEM;
        ret.pt_set = NULL;
    }
    else if (
        (a.alias_type == MUST_MEM && contains err) || b.val_type == MUST_NAI
    ) {
        ret.alias_type = MUST_MEM;
        ret.pt_set = array with err;
    }
    else if (a.alias_type == MAY_ALL_MEM) {
        ret.alias_type = MAY_ALL_MEM;
        ret.pt_set = a.pt_set;
        return ret;
    }
    else if (a.alias_type == MUST_MEM) {
        MemStateObj *obj = first obj from a.pt_set;

        if (b.val_type == MUST_VAL) {
            ret.alias_type = MUST_MEM;

            Val offset = obj->mem.offset op first val from b.val_set;
            MemStateObj *new = get_mem(obj->mem.base, offset, state);
            if (!new) new = create_mem(obj->mem.base, offset, state);

            ret.pt_set = array with new;
        }
        else {
            ret.alias_type = MAY_MEM;
            ret.pt_set = array(sizeof(MemStateObj *), i);

            Array *val_set = b.val_set;
            Val *arr = (Val *) val_set->arr;
            for (int i = 0; i < val_set->size; i++) {
                if (arr[i].type == NAI) {
                    MemStateObj *err = get_err_mem(state);
                    insert_pt_set(&ret, err);
                }
                else if (arr[i].type == ALL_VAL) {
                    Val offset = arr[i];
                    MemStateObj *new = get_mem(obj->mem.base, offset, state);
                    if (!new) new = create_mem(obj->mem.base, offset, state);
                    insert_pt_set(&ret, new);
                }
                else if (arr[i].type == VAL) {
                    Val offset = obj->mem.offset op arr[i];
                    MemStateObj *new = get_mem(obj->mem.base, offset, state);
                    if (!new) new = create_mem(obj->mem.base, offset, state);
                    insert_pt_set(&ret, new);
                }
            }
        }
    }
    else {
        ret.alias_type = MAY_MEM;
        ret.pt_set = array(sizeof(MemStateObj *), i);

        Array *pt_set = a.pt_set;
        MemStateObj **mem_arr = (MemStateObj **) pt_set->arr;

        Array *val_set = b.val_set;
        Val *val_arr = (Val *) val_set->arr;

        if (err in mem_arr || NAI in val_arr) {
            MemStateObj *err = get_err_mem(state);
            insert_pt_set(&ret, err);
        }

        for (obj in mem_arr other than err) {
            for (val in val_arr other than NAI) {
                Val offset = obj->mem.offset op val;
                MemStateObj *new = get_mem(obj->mem.base, offset, state);
                if (!new) new = create_mem(obj->mem.base, offset, state);
                insert_pt_set(&ret, new);
            }
        }
    }
    return ret;
}

ValSet _op_val(ValSet a, ValSet b, Op op) {
    if (a.val_type == NO_VAL || b.val_type == NO_VAL) return a;
    else if (a.val_type == MUST_NAI) return a;
    else if (b.val_type == MUST_NAI) return b;
    else if (a.val_type == MAY_ALL_NAI) return a;
    else if (b.val_type == MAY_ALL_NAI) return b;

    ValSet v;

    if (a.val_type == MUST_VAL) {
        v.val_type = b.val_type;
        if (NAI in b vals) add NAI to v.val_set;
        if (ALL_VAL in b vals) add ALL_VAL to v.val_set;
        for (val other than NAI and ALL_VAL in b vals) {
            insert_val_set(&v, a_val op val);
        }
        return v;
    }
    else if (a_val or b_val = NAI && a_val or b_val == ALL_VAL) {
        v.val_type = MAY_ALL_NAI;
        v.val_set = array with NAI and ALL_VAL;
        return v;
    } 
    else if (a_val or b_val == ALL_VAL) {
        v.val_type = MAY_ALL_VAL;
        v.val_set = array with ALL_VAL;
        return v;
    }
    else if (a_val or b_val = NAI) {
        v.val_type = MAY_NAI;
        v.val_set = array with NAI;
    }

    for (a_val in a vals other than NAI, ALL_VAL) {
        for (b_val in b vals other than NAI, ALL_VAL) {
            insert_val_set(&v, a_val op b_val);
        }
    }
    return v;
}

/* Transfer function for x = y op z:
 * Needs to handle the case where y has mem type and
 * z has int type, y has int type and z has mem type,
 * and both are int types. Note that since y, z represent
 * operands, y or z must have mem type, or both have
 * non-mem types. */
// TODO: be careful with temp array creation.
bool tf_arith(Inst *inst, AliasState *state) {
    bool changed = false;

    Operand lhs = inst->lhs;
    Operand rhs1 = inst->rhs1;
    Operand rhs2 = inst->rhs2;

    /* Get LHS's memory object. */
    Val offset; offset.type = VAL; offset.value = 0;
    MemStateObj *l = get_mem(lhs.var, offset, state);
    if (!l) l = create_mem(lhs.var, offset, state);

    MemStateObj t;

    /* If both rhs operands are constants... */
    if (rhs1.type == CONST && rhs2.type == CONST) {
        t.pt_set.alias_type = MUST_MEM;
        t.pt_set.pt_set = get_err_mem(state);

        if (rhs1.constant.type == INT && rhs2.constant.type == INT) {
            t.val_set.val_type = MUST_VAL;
            t.val_set.val_set = array containing rhs1.constant.value + rh2.constant.value;
        }
        else {
            t.val_set.val_type = MUST_NAI;
            t.val_set.val_set = array containing NAI;
        }
    }
    /* If either rhs operand is a constant... */
    else if (rhs1.type == CONST || rhs2.type == CONST) {
        Operand c = rhs1.type == CONST ? rhs1 : rhs2;
        Operand v = rhs1.type == CONST ? rhs2 : rhs1;

        /* If the other rhs operand is an int variable... */
        if (v.var->type == INT) {
            t.pt_set.alias_type = MUST_MEM;
            t.pt_set.pt_set = get_err_mem(state);

            if (c.constant.type == INT) {
                t.val_set.val_type = MUST_VAL;
                t.val_set.val_set = array containing c.constant.value + v.var.value;
            }
            else {
                t.val_set.val_type = MUST_NAI;
                t.val_set.val_set = array containing NAI;
            }
        }
        /* If the other rhs operand is a mem variable... */
        else if (v.var->type == MEM) {
            ValSet val_set;

            if (rhs1.constant.type != INT) {
                val_set.val_type = MUST_NAI;
                val_set.val_set = array with err;
            }
            else {
                val_set.val_type = MUST_VAL;
                val_set.val_set = array with c.constant.value;
            }

            MemStateObj *r = get_mem(v.var, offset, state);
            if (!r) r = create_mem(v.var, offset, state);

            t.pt_set = _op_mem(r->pt_set, val_set, op, state);
            t.val_set = _op_val(r->val_set, val_set, op);
        }
        /* Otherwise, error. */
        else {
            t.pt_set.alias_type = MUST_MEM;
            t.pt_set.pt_set = get_err_mem(state);
            t.val_set.val_type = MUST_NAI;
            t.val_set.val_set = array with NAI;
        }
    }
    /* Else if both rhs vars are variables... */
    else {
        /* If both have int type... */
        if (rhs1.var->type == INT && rhs2.var->type == INT) {
            t.pt_set.alias_type = MUST_MEM;
            t.pt_set.pt_set = get_err_mem(state);

            MemStateObj *r1 = get_mem(rhs1.var, offset, state);
            if (!r1) r1 = create_mem(rhs1.var, offset, state);

            MemStateObj *r2 = get_mem(rhs2.var, offset, state);
            if (!r2) r2 = create_mem(rhs2.var, offset, state);

            t.val_set = _op_val(r1->val_set, r2->val_set, op);
        }
        /* If one has int type... */
        else if (rhs1.var->type == INT || rhs2.var->type == INT) {
            Operand v = rhs1.var->type == INT ? rhs1 : rhs2;
            Operand m = rhs1.var->type == INT ? rhs2 : rhs1;

            MemStateObj *v_obj = get_mem(v.var, offset, state);
            if (!v_obj) v_obj = create_mem(v.var, offset, state);

            MemStateObj *m_obj = get_mem(m.var, offset, state);
            if (!m_obj) m_obj = create_mem(m.var, offset, state);

            t.pt_set = _op_mem(m_obj->pt_set, v_obj->val_set, state);
            t.val_set = _op_val(m_obj->val_set, v_obj->val_set, op);
        }
        /* Otherwise, error. */
        else {
            t.pt_set.alias_type = MUST_MEM;
            t.pt_set.pt_set = get_err_mem(state);
            t.val_set.val_type = MUST_NAI;
            t.val_set.val_set = array with NAI;
        }
    }

    changed = changed || meet_pt_set(l, &t, state);
    changed = changed || meet_val_set(l, &t, state);

    return changed;
}

bool tf(void *b, void *s) {
    Block *block = (Block *) b;
    AliasState *state = (AliasState *) s;

    bool changed = false;

    for (inst in block->insts) {
        switch (inst->type) {
            case INST_COPY:
                changed = changed || tf_copy(inst, state);
                break;
            case INST_ADDR:
                changed = changed || tf_addr(inst, state);
                break;
            case INST_RHS_DEREF:
                changed = changed || tf_rhs_deref(inst, state);
                break;
            case INST_LHS_DEREF:
                changed = changed || tf_lhs_deref(inst, state);
                break;
            case INST_ARITH:
                changed = changed || tf_arith(inst, state);
                break;
            // TODO: other inst types
            default:
        }
    }

    return changed;
}

AliasState *alias_analysis(Array *blocks) {
    void *state = dfa_generic(blocks, init_state, tf);
    return (AliasState *) state;
}
