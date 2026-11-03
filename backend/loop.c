/* Loop discovery.
 * If a block heads a loop, the loop must be the
 * innermost loop containing the block.
 * DFS does NOT guarantee parent-child ordering.
 * [Example]
 * 
 * Option 1: naive.
 * Use dominator tree to identify loop heads, then
 * for each loop head, use BFS to find all block
 * members. O(n^2). Have a separate pass to build parent-
 * child relationship and find innermost_loop,
 * namely, sort loop by member size, for each loop,
 * check if it's the parent of some other loops,
 * then, for each block in loop, if innermost_loop is not
 * tagged, tag it with the current loop.
 * O(k log k) + O(k^2) + O(n^2).
 * 
 * 
 * An optimized solution:
 * Inspired by LLVM, which uses the algorithm from
 * A New Algorithm for Identifying Loops in
 * Decompilation (Wei et al., 2007).
 * This solution does not use a dominator tree.
 * Pass 1: run DFS in reverse postorder to tag
 * innermost_loop_header[b] for all blocks. The
 * innermost_loop_header[b] when b is a loop header
 * is tagged to the parent loop header.
 * Pass 2: Tag parent-child relationships and
 * update innermost_loop_header[b] for each header
 * as itself.
 * Pass 3: build loop block membership. For each block,
 * add it to all loops that contain it.
 * 
 * Need proof: tag_header is correct. No other scenarios
 * of graph state at each iteration of DFS.
 */

Array<Block *> innermost_loop_header(n_blocks);
Array<size_t> path_ind(n_blocks);  // 0 means not on current path
Array<bool> visited(n_blocks);
Array<Loop *> loop(n_blocks);

/* Assign each block to the header of the innermost loop
 * that contains the block. */
Block *tag_header_dfs(Block *block, int depth) {

    visited[block->ind] = true;
    path_ind[block->ind] = depth;

    for child in block->bl0, block->bl1 {

        if (!child) continue;

        /* If child is not visited, visit the child,
         * and update innermost loop header based
         * on the innermost loop header of the child. */
        if (!visited[child->ind]) {
            Block *header = tag_header_dfs(child, depth + 1);
            tag_header(block, header);
        }
        /* If child is visited on the DFS path,
         * mark the child as a loop header. */
        else if (path_ind[child->ind]) {
            if (!loop[child->ind]) {
                loop[child->ind] = new Loop(child);  // loop->header = child
            }
            tag_header(block, child);
        }
        /* If child is not on the DFS path but has
         * no loop header, skip. */
        else if (!innermost_loop_header[child]) { /* Skip */ }
        else {
            Block *header = innermost_loop_header[child];
            if (path_ind[header->ind]) tag_header(block, header);
            else {
                /* Loop is irreducible. Raise error. */
                raise error();
            }
        }
    }

    path_ind[block->ind] = 0;
    return innermost_loop_header[block->ind];
}

/* Tag header on the fly. Intuition:
 * 1. On any DFS path, if A, B are both loop headers
 * and A is deeper than B, and we know one loop
 * must be nested in the other, loop A must be nested
 * in loop B.
 * 2. If block A belongs to both loop 1 and loop 2,
 * either loop 1 is nested in loop 2 or loop 2 is nested
 * in loop 1.
 * 3. If block A belongs to both loop 1 and loop 2,
 * headed by B and C, B and C must be on the DFS path
 * of A.
 * Based on these properties, whenever we identify that
 * a block belongs to multiple loops, we can update
 * the parent-child relationship of these loops. To
 * record the parent-child relationship, we tag
 * innermost_loop_header[loop header] to be the header
 * of the parent loop. This is different from
 * innermost_loop_header[non loop header], which is tagged
 * the header of the current loop. */
void tag_header(Block *a, Block *b) {
    if (a == b || !b) return;

    while (innermost_loop_header[a->ind]) {
        Block *curr_header = innermost_loop_header[a->ind];

        /* Skip if the current block is a loop header or
         * does not belong to any loop. If the current
         * block is a loop header, its innermost_loop_header
         * must have already been tagged. */
        if (curr_header == b) return;

        /* If a's current loop header (curr_header)
         * is shallower than the candidate loop header (b)
         * on the dfs path, set curr_header to b instead,
         * and set curr_header as a candidate loop header
         * to b. */
        if (path_ind[curr_header->ind] < path_ind[b->ind]) {
            innermost_loop_header[a->ind] = b;
            a = b;
            b = curr_header;
        }
        else a = curr_header;
    }

    innermost_loop_header[a->ind] = b;
}

tag_header_dfs(entry, 1);

/* Tag parent-child relationships. */
for (int i = 0; i < n_loops; i++) {
    Block *header = loop_pool[i].header;
    Loop *l = &loop_pool[i];

    l->parent = loop[innermost_loop_header[header]->ind];
    if (l->parent) l->parent->children.add(l);

    innermost_loop_header[header] = header;
}

/* Tag loop membership. */
for (int i = 0; i < n_blocks; i++) {
    Block *block = &block_pool[i];  // Assume pool is only reachable blocks
    Block *header = innermost_loop_header[i];
    if (!header) continue;

    Loop *curr = loop[header];
    while (curr) {
        curr->blocks.add(block);
        curr = curr->parent;
    }
}

/* Identify exits. If a loop block branches to a block
 * outside the current block set, then the block is an exit. */
for block in blocks {
    Loop *loop = loop[block->ind];
    while (
        loop
        && (
            block->bl0 not in loop->blocks
            || block->bl1 not in loop->blocks
        )
    ) {
        loop->exits.add(block);
        loop = loop->parent;
    }
}
