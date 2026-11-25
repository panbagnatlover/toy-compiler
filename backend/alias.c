
bool interval_overlap(int offset_a, int size_a, int offset_b, int size_b) {
    int l_offset, r_offset;
    int l_size, r_size;
    if (offset_a <= offset_b) {
        l_offset = offset_a;
        r_offset = offset_b;
        l_size = size_a;
        r_size = size_b;
    }
    else {
        l_offset = offset_b;
        r_offset = offset_a;
        l_size = size_b;
        r_size = size_a;
    }
    return l_offset + l_size > r_offset && l_offset < r_offset + r_size;
}

/* Given two VMems and the number of bytes written
 * at these locations, return MUST_ALIAS, MAY_ALIAS,
 or NO_ALIAS.
 * Note: if VMems are [base, #1], [base, #2], etc., 
 * by our analysis framework, if base, #1 and base, #2
 * overlap, their pt sets overlap. Converse is not true.
 * Need proof. */
AliasType alias(VMem *a, VMem *b) {
    PointToSet a_pt = a->pt_set;
    PointToSet b_pt = b->pt_set;

    /* If either none, raise error. */
    /* If both must:and share base, and overlap, MUST. */
    /* If either may, and share base, and overlap, MAY. */

    /* If either a or b have NO MEM, raise error. */
    if (a_pt.pt_type == NO_MEM || b_pt.pt_type == NO_MEM)
        raise error;
    /* If either a or b MUST point to ERR, return NO ALIAS. */
    if (a_pt.pt_type == MUST_MEM && err in a_pt) return NO_ALIAS;
    if (b_pt.pt_type == MUST_MEM && err in b_pt) return NO_ALIAS;
    /* If either a or b MAY point to ALL_MEM, return MAY_ALIAS. */
    if (a_pt.pt_type == MAY_ALL_MEM || b_pt.pt_type == MAY_ALL_MEM)
        return MAY_ALIAS;
    /* If both MUST point to some memory... */
    if (a_pt.pt_type == MUST_MEM && b_pt.pt_type == MUST_MEM) {
        MemStateObj *a_obj = get first obj from a_pt;
        MemStateObj *b_obj = get first obj from b_pt;
        /* If pointed-to memories do not share base, return NO ALIAS. */
        if (a_obj->mem.base != b_obj->mem.base) return NO_ALIAS;
        /* If pointed-to memories share base and intervals overlap,
         * return MUST_ALIAS. Else, NO_ALIAS. */
        if (interval_overlap(a_obj->mem.offset.value, size_a, b_obj->mem.offset.value, size_b))
            return MUST_ALIAS;
        return NO_ALIAS;
    }
    /* If either MAY point to some memory... */
    else {
        if (a_pt.pt_type == MUST_MEM || b_pt.pt_type == MUST_MEM) {
            MemStateObj *must_obj;
            Array *may_objs;
            int l_size, r_size;
            if (a_pt.pt_type == MUST_MEM) {
                must_obj = (MemStateObj *) a_pt.pt_set;
                may_objs = b_pt.pt_set;
                l_size = a->size;
                r_size = b->size;
            }
            else {
                must_obj = (MemStateObj *) b_pt.pt_set;
                may_objs = a_pt.pt_set;
                l_size = b->size;
                r_size = a->size;
            }
            for (obj in may_objs) {
                if (may_obj->mem.base != must_obj->mem.base) continue;
                if (may_obj->mem.offset.type == ALL_VAL) return MAY_ALIAS;
                if (
                    interval_overlap(
                        must_obj->mem.offset.value, l_size,
                        may_obj->mem.offset.value, r_size
                    )
                ) return MAY_ALIAS;
            }
            return NO_ALIAS;
        }
        
        Array *a_objs = a_pt.pt_set;
        Array *b_objs = b_pt.pt_set;

        for (a_obj in a_objs) {
            for (b_obj in b_objs) {
                if (a_obj->mem.base != b_obj->mem.base) continue;
                if (a_obj->mem.offset.type == ALL_VAL) return MAY_ALIAS;
                if (b_obj->mem.offset.type == ALL_VAL) return MAY_ALIAS;
                if (
                    interval_overlap(
                        a_obj->mem.offset.value, a->size,
                        b_obj->mem.offset.value, b->size
                    )
                ) return MAY_ALIAS;
            }
        }
        return NO_ALIAS;
    }
}
