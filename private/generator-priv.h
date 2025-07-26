#pragma once

#include "generator.h"
#include "object.h"
#include "scanner.h"
#include <stdbool.h>

void declaration(vmake_gen *gen);
void statement(vmake_gen *gen);
void print_statement(vmake_gen *gen);
void include_statement(vmake_gen *gen);
void expression_statement(vmake_gen *gen);
vmake_value expression(vmake_gen *gen);
vmake_value assigment(vmake_gen *gen);
vmake_value equality(vmake_gen *gen);
vmake_value comparison(vmake_gen *gen);
vmake_value term(vmake_gen *gen);
vmake_value factor(vmake_gen *gen);
vmake_value unary(vmake_gen *gen);
vmake_value subscript(vmake_gen *gen);
vmake_value call(vmake_gen *gen);
vmake_value primary(vmake_gen *gen);
vmake_arguments arguments(vmake_gen *gen);
vmake_value grouping(vmake_gen *gen);
vmake_value array(vmake_gen *gen);
vmake_value number(vmake_gen *gen);
vmake_value string(vmake_gen *gen);
vmake_value identifier_variable(vmake_gen *gen);
vmake_value identifier_string(vmake_gen *gen);

vmake_value assign_variable(vmake_gen *gen, vmake_value *lhs, vmake_value rhs);
vmake_value call_native(vmake_gen *gen, vmake_value val, vmake_arguments *args);
vmake_value call_method(vmake_gen *gen, vmake_value val, vmake_obj_instance *caller,
                        vmake_arguments *args);
vmake_value *resolve_variable(vmake_gen *gen, vmake_token name);
vmake_value *resolve_local(vmake_gen *gen, vmake_token name);
vmake_value *resolve_global(vmake_gen *gen, vmake_token name);
vmake_value *create_local(vmake_gen *gen, vmake_token name);
vmake_value *create_global(vmake_gen *gen, vmake_token name);
