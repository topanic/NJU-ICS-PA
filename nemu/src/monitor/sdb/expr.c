/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <memory/paddr.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <stdio.h>
#include <string.h>

enum {
    TK_NOTYPE = 64,
    TK_EQ,
    NUM,
    HEX,
    TK_UEQ,
    REG,
    DEREF,
    MINUS  // 取负
};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {//这里面不要有字符的type，因为标识从A开始
        {"0x[0-9A-Fa-f]+", HEX}, //16进制数字
        {"\\$[0-9a-z]+",   REG},//寄存器
        {"[0-9]+",         NUM},       // 数字
        {"\\(",            '('},         // 左括号
        {"\\)",            ')'},         // 右括号
        {"\\+",            '+'},         // plus
        {"\\-",            '-'},         // sub
        {"\\*",            '*'},         // mul
        {"\\/",            '/'},         // divide
        {" +",             TK_NOTYPE},    // spaces
        {"==",             TK_EQ},        // equal
        {"!=",             TK_UEQ},
        {"&&",             '&'},
        {"\\|\\|",         '|'}
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
    int i;
    char error_msg[128];
    int ret;

    for (i = 0; i < NR_REGEX; i++) {
        ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
        if (ret != 0) {
            regerror(ret, &re[i], error_msg, 128);
            panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
        }
    }
}

typedef struct token {
    int type;
    char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used)) = 0;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;

    nr_token = 0;

    while (e[position] != '\0') {
        /* Try all rules one by one. */
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;

                Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
                    i, rules[i].regex, position, substr_len, substr_len, substr_start);

                position += substr_len;

                /* TODO: Now a new token is recognized with rules[i]. Add codes
                 * to record the token in the array `tokens'. For certain types
                 * of tokens, some extra actions should be performed.
                 */

                switch (rules[i].token_type) {
                    case TK_NOTYPE:
                        break;
                    case TK_NUM:
                        for (int j = 0; j < substr_len; j++) {
                            tokens[nr_token].str[j] = *(substr_start + j);
                        }
                        tokens[nr_token].type = TK_NUM;
                        nr_token++;
                        break;
                    case TK_EQ:
                    case TK_UNEQ:
                    case TK_AND:
                        tokens[nr_token].str[0] = *(substr_start);
                        tokens[nr_token].str[1] = *(substr_start + 1);
                        tokens[nr_token].type = rules[i].token_type;
                        nr_token++;
                        break;
                    case TK_PLUS:
                    case TK_MINUS:
                    case TK_MUL:
                    case TK_DIV:
                    case TK_R:
                    case TK_L:
                        tokens[nr_token].str[0] = *(substr_start);
                        tokens[nr_token].type = rules[i].token_type;
                        nr_token++;
                        break;
                    default:
                        break;
                }
                break;
            }
        }

        if (i == NR_REGEX) {
            printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
            return false;
        }
    }

    return true;
}


word_t expr(char *e, bool *success) {
    if (!make_token(e)) {
        *success = false;
        return 0;
    }

    /* TODO: Insert codes to evaluate the expression. */
    TODO();

    return 0;
}
