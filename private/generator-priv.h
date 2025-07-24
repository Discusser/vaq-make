#pragma once

#include "array.h"
#include "generator.h"
#include "scanner.h"
#include <stdbool.h>

void declaration(vaq_make_gen *gen);
void statement(vaq_make_gen *gen);
void print_statement(vaq_make_gen *gen);
void expression_statement(vaq_make_gen *gen);
vaq_make_value expression(vaq_make_gen *gen);
vaq_make_value assigment(vaq_make_gen *gen);
vaq_make_value equality(vaq_make_gen *gen);
vaq_make_value comparison(vaq_make_gen *gen);
vaq_make_value term(vaq_make_gen *gen);
vaq_make_value factor(vaq_make_gen *gen);
vaq_make_value unary(vaq_make_gen *gen);
vaq_make_value call(vaq_make_gen *gen);
vaq_make_value primary(vaq_make_gen *gen);
vaq_make_value_array arguments(vaq_make_gen *gen);
vaq_make_value grouping(vaq_make_gen *gen);
vaq_make_value number(vaq_make_gen *gen);
vaq_make_value string(vaq_make_gen *gen);
vaq_make_value identifier(vaq_make_gen *gen);

vaq_make_value assign_variable(vaq_make_gen *gen, vaq_make_value *lhs, vaq_make_value rhs);
vaq_make_value call_value(vaq_make_gen *gen, vaq_make_value val, vaq_make_value_array args);
vaq_make_value *resolve_variable(vaq_make_gen *gen, vaq_make_token name);
