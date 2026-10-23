/* SSA-lifting with union find. Identify webs and
 * ensure each distinct web has its own distinct variable.
 * Output webs are not strictly SSA since this design
 * does not insert Phi nodes. */

#include <stdlib.h>

/* DFA - find defs for each operand */

Array<size_t> root(operand_pool_size); // Heap array
Array<size_t> rank(operand_pool_size);
for (int i; i < root.size(); i++) root[i] = i;
for (int i; i < rank.size(); i++) rank[i] = 0;

void find(size_t ind) {
    size_t orig_ind = ind;
    while (ind != root[ind]) {
        ind = root[ind];
    }
    root[orig_ind] = ind;
}

void join(size_t ind1, size_t ind2) {
    size_t root1 = find(ind1);
    size_t root2 = find(ind2);
    if (root1 == root2) return;
    if (rank[root1] > rank[root2]) root[root2] = root1;
    else if (rank[root1] < rank[root2]) root[root1] = root2;
    else {
        root[root2] = root1;
        rank[root1]++;
    }
}

for each block {
    for each instruction {
        find defs;
        for each operand {
            for each relevant def {
                join(operand->ind, def->lhs->ind);
            }
        }
    }
}

Array<bool> root_assigned(operand_pool_size);
Array<bool> var_claimed(variable_pool_size);

/* TODO: Might be easier to loop through each operand in pool */
for each block {
    for each instruction {
        for each operand {
            size_t root_ind = find(operand->ind);
            Operand *root_op = &operand_pool[root_ind];

            /* If the root is not yet assigned a variable,
             * assign it. */
            if (!root_assigned[root_ind]) {
                size_t var_ind = root_op->var->id;

                /* If the variable tied to the root operand
                 * is already claimed, create a new one.
                 * No need to mark the new variable as claimed
                 * since we only need to mark the old ones. */
                if (var_claimed[var_ind]) root_op->var = newvar();
                else var_claimed[var_ind] = true;

                root_assigned[root_ind] = true;
            }

            operand->var = root_op->var;
        }
    }
}
