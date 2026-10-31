/* expr.h - home-screen style expression evaluator with sig figs and units.
 *   (4.82*3.0100)/(15.2-2.11)   2.50E3*1.2/3.456kg   +5 (= Ans+5)
 * Operators: + - * / ^ ( ) ², (-) negation, E exponent, implicit multiply.
 * A unit may follow any number or ")". Each step is rounded per the
 * sig-fig rules when round_steps is set. */
#ifndef CHEM_EXPR_H
#define CHEM_EXPR_H

#include "units.h"
#include "work.h"

enum { EXPR_OK = 0, EXPR_SYNTAX, EXPR_UNITS, EXPR_DIV0, EXPR_EMPTY };

typedef struct {
    qty_t ans;            /* previous answer, used when a line starts with an operator */
    bool has_ans;
    bool round_steps;
    uint8_t err;          /* EXPR_* */
    uint8_t errpos;       /* offset of the error in the input */
    work_t *work;         /* optional: one line per operation */
} expr_ctx_t;

bool expr_eval(const char *s, expr_ctx_t *ctx, qty_t *out);
/* true if the line starts with a binary operator (then Ans is prepended) */
bool expr_starts_with_op(const char *s);
const char *expr_err_str(uint8_t err);

#endif
