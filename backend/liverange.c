

        /* Store liveness info in Slot objects. */
        for (int i = 0; i < blocks->size; i++) {
            Block *block = blocks[i];
            LivenessDFAObject *obj = &objs[i];
            BitArray *live = obj->out;

            /* Create a new slot object from the live out set. */
            Slot *slot = create_slot(c->slots);
            slot->live_set = live;
            slot->inst = NULL;
            push_dll_elem(slot, &(c->block_to_slots[block->ind]));

            BitArray *tmp_def = bitarray();
            BitArray *tmp_use = bitarray();

            /* For each instruction, create a corresponding
             * slot object from liveness information and
             * set up linkage. The index to each slot object
             * represents the order of the instruction in a
             * block. */
            for (each inst backwards) {
                set_def_vreg(inst, tmp_def);
                set_use_vreg(inst, tmp_use);
                live = tmp_use U (live - tmp_def);

                reset_bitarray(tmp_def);
                reset_bitarray(tmp_use);

                Slot *slot = create_slot(c->slots);
                slot->live_set = live;
                slot->inst = inst;
                c->inst_to_slot[inst->ind] = slot;
                insert_dll_elem(slot, NULL, &(c->block_to_slots[block->ind]);
            }
        }