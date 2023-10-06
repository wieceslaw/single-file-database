//
// Created by vyach on 06.10.2023.
//

#ifndef LLP_LAB1_WHERE_H
#define LLP_LAB1_WHERE_H

typedef enum {
    WHERE_COND_AND = 0,
    WHERE_COND_OR = 1,
    WHERE_COND_NOT = 2,
    WHERE_COND_COMPARE = 3,
} where_condition_type;

typedef enum {
    OPERAND_LIT = 0,
    OPERAND_COL = 1,
} operand_type;

#endif //LLP_LAB1_WHERE_H
