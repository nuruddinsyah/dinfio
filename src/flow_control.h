/*
------------------------------------------------------------
 Dinfio Programming Language
 Version: 3.1
------------------------------------------------------------
 By: Muhammad Faruq Nuruddinsyah
 Copyright (C) 2014-2021. All Rights Reserved.
------------------------------------------------------------
 Platform: Linux, macOS, Windows
------------------------------------------------------------
 Flow Controls
------------------------------------------------------------
*/

void loop_for(AST* body, uint_fast8_t& additional_header, uint_fast32_t& endclause, uint_fast32_t& caller_id) {
    AST* a_counter = ((AST_Parameter*) body)->__parameters[0];
    AST* a_start = ((AST_Parameter*) body)->__parameters[1];
    AST* a_end = ((AST_Parameter*) body)->__parameters[2];

    double step = 0;
    uint_fast32_t start_i = __cur_i;

    if (a_counter->__type != __AST_VARIABLE__) error_message("Parameter 'counter' must be a variable");

    if (additional_header == __H_FOR_WITH_STEP__) {
        AST* st = ((AST_Parameter*) body)->__parameters[3];
        DataType* dst = get_value(st, caller_id);
        step = dst->__value_double;

        if (step == 0) error_message("Unable to use 0 in the parameter 'step'");
        remove_garbage(st, dst);
    } else {
        step = 1;
    }

    DataType* d_start = get_value(a_start, caller_id);
    DataType* d_end = get_value(a_end, caller_id);

    double start = d_start->__value_double;
    double end = d_end->__value_double;

    remove_garbage(a_start, d_start);
    remove_garbage(a_end, d_end);

    AST_Variable* e = (AST_Variable*) a_counter;
    DataType* counter;
    string c = to_string(caller_id);

    if (__variables.count(c + e->__identifier) == 1) {
        counter = __variables[c + e->__identifier];
        counter->__type = __TYPE_DOUBLE__;
    } else {
        counter = new DataType(__TYPE_DOUBLE__);
        __variables[c + e->__identifier] = counter;
    }

    if (step > 0) {
        for (double i = start; i <= end; i += step) {
            if (__stop_loop || __stop_procedure) break;
            
            counter->__value_double = i;
            walk(start_i + 1, caller_id);
        }
    } else {
        for (double i = start; i >= end; i += step) {
            if (__stop_loop || __stop_procedure) break;

            counter->__value_double = i;
            walk(start_i + 1, caller_id);
        }
    }

    __stop_loop = false;
    walk(endclause + 1, caller_id);
}

void loop_while(AST* body, uint_fast32_t& endclause, uint_fast32_t& caller_id) {
    uint_fast32_t start_i = __cur_i;
    DataType* dc = get_value(body, caller_id);
    bool condition = dc->__value_bool;
    remove_garbage(body, dc);

    while (condition) {
        if (__stop_loop || __stop_procedure) break;
        walk(start_i + 1, caller_id);

        dc = get_value(body, caller_id);
        condition = dc->__value_bool;
        remove_garbage(body, dc);
    }

    __stop_loop = false;
    walk(endclause + 1, caller_id);
}

void branch_if(AST* body, uint_fast32_t& endclause, uint_fast32_t& endclause2, bool is_elseif, uint_fast32_t& caller_id) {
    uint_fast32_t start_i = __cur_i;
    DataType* dc = get_value(body, caller_id);
    bool condition = dc->__value_bool;
    remove_garbage(body, dc);

    if (condition) {
        walk(start_i + 1, caller_id);
    } else {
        if (endclause2 != 0) {
            Code* c = __codes[endclause2];

            if (c->__header == __H_ELSE_IF__) {
                __cur_i = endclause2;
                branch_if(c->__body, c->__endclause, c->__endclause2, true, caller_id);
            } else {
                walk(endclause2 + 1, caller_id);
            }
        }
    }

    if (!is_elseif) walk(endclause + 1, caller_id);
}

void parse_flow_control() {
    uint_fast32_t count = __flow_controls.size();

    for (uint_fast32_t i = 0; i < count; i++) {
        Code* c = __flow_controls.at(i);
        __cur_i = c->__index;

        if (c->__header == __H_IF__ || c->__header == __H_ELSE_IF__) {
            uint_fast32_t n_if = 0;
            uint_fast32_t i_else = 0;
            uint_fast32_t i_endif = 0;

            for (uint_fast32_t j = i + 1; j < count; j++) {
                if (j >= count) break;
                Code* d = __flow_controls.at(j);

                if (d->__header == __H_STOP__) break;
                if (d->__header == __H_IF__) n_if++;

                if (d->__header == __H_END_IF__) {
                    if (n_if == 0) {
                        i_endif = d->__index;
                        break;
                    }

                    if (n_if > 0) n_if--;
                }

                if (n_if == 0 && (d->__header == __H_ELSE__ || d->__header == __H_ELSE_IF__) && i_else == 0) i_else = d->__index;
            }

            if (i_endif == 0) error_message("No 'endif' found");

            c->__endclause = i_endif;
            c->__endclause2 = i_else;
        } else if (c->__header == __H_FOR__) {
            uint_fast32_t n_for = 0;
            uint_fast32_t i_endfor = 0;

            for (uint_fast32_t j = i + 1; j < count; j++) {
                if (j >= count) break;
                Code* d = __flow_controls.at(j);

                if (d->__header == __H_STOP__) break;
                if (d->__header == __H_FOR__) n_for++;

                if (d->__header == __H_END_FOR__) {
                    if (n_for == 0) {
                        i_endfor = d->__index;
                        break;
                    }

                    if (n_for > 0) n_for--;
                }
            }

            if (i_endfor == 0) error_message("No 'endfor' found");

            c->__endclause = i_endfor;
        } else if (c->__header == __H_WHILE__) {
            uint_fast32_t n_while = 0;
            uint_fast32_t i_endwhile = 0;

            for (uint_fast32_t j = i + 1; j < count; j++) {
                if (j >= count) break;
                Code* d = __flow_controls.at(j);

                if (d->__header == __H_STOP__) break;
                if (d->__header == __H_WHILE__) n_while++;

                if (d->__header == __H_END_WHILE__) {
                    if (n_while == 0) {
                        i_endwhile = d->__index;
                        break;
                    }

                    if (n_while > 0) n_while--;
                }
            }

            if (i_endwhile == 0) error_message("No 'endwhile' found");

            c->__endclause = i_endwhile;
        }
    }
}
