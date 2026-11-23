/* Peephole rules to fold instructions during
 * instruction selection.
 * For now, keep the # of rules under 256. */
#ifndef H_RULE
#define H_RULE

#include "inst.h"

typedef bool (*EligibilityFunc)(Inst *inst);
typedef void (*CombineFunc)(Inst *inst, Array *changed, Array *inst_info);  // changed and inst_info are fixed arrays

typedef struct {
    EligibilityFunc check_eligibility;
    CombineFunc combine;
} Rule;

typedef struct {
    Rule **rules;
    uint8_t size;
} RuleSet;

/* Need:
 * Define glboal rule objects.
 * For each inst type, have a list of rule objects.
 * The list should also be global.
 * e.g., case TYPE1: get rule obj list 1, etc.
 * 
 */

RuleSet get_rule_set(Inst *inst);

#endif
