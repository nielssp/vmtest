#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ins.h"

#define BIG_ENDIAN 0

struct offset {
  size_t offset;
  struct offset *next;
};

struct label {
  char *name;
  char seen;
  size_t target;
  struct offset *pending;
  struct label *next;
};

void skip_ws(FILE *f) {
  int c = fgetc(f);
  while (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ';') {
    if (c == ';') {
      c = fgetc(f);
      while (c != '\n' && c != EOF) {
        c = fgetc(f);
      }
      continue;
    }
    c = fgetc(f);
  }
  ungetc(c, f);
}

char *read_token(FILE *f) {
  char *token = malloc(32);
  int l = 0;
  while (l < 31) {
    int c = fgetc(f);
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == EOF || c == ':') {
      ungetc(c, f);
      break;
    }
    token[l++] = (char)c;
  }
  if (l == 0) {
    free(token);
    return NULL;
  }
  token[l] = 0;
  return token;
}

int contains_label(struct label *stack, char *name, size_t target) {
  if (!stack) {
    return 0;
  }
  if (strcmp(name, stack->name) == 0) {
    if (stack->seen) {
      printf("multiple definitions of label: %s\n", name);
    }
    stack->target = target;
    stack->seen = 1;
    return 1;
  } else {
    return contains_label(stack->next, name, target);
  }
}

struct label *push_label(struct label *stack, char *name, size_t target) {
  if (contains_label(stack, name, target)) {
    return stack;
  }
  struct label *new = malloc(sizeof(struct label));
  new->name = name;
  new->target = target;
  new->seen = 1;
  new->pending = NULL;
  new->next = stack;
  return new;
}

void push_pending(struct label *stack, size_t prog_p) {
  struct offset *off = malloc(sizeof(struct offset));
  off->offset = prog_p;
  off->next = stack->pending;
  stack->pending = off;
}

int get_label(struct label *stack, char *name, size_t prog_p, size_t *target) {
  if (strcmp(name, stack->name) == 0) {
    if (!stack->seen) {
      push_pending(stack, prog_p);
    }
    *target = stack->target;
    return 1;
  } else if (stack->next) {
    return get_label(stack->next, name, prog_p, target);
  }
  return 0;
}

size_t get_or_add_label(struct label **stack, char *name, size_t prog_p) {
  size_t target;
  if (*stack && get_label(*stack, name, prog_p, &target)) {
    return target;
  } else {
    struct label *new = malloc(sizeof(struct label));
    new->name = name;
    new->target = 0;
    new->seen = 0;
    new->pending = NULL;
    new->next = *stack;
    *stack = new;
    push_pending(*stack, prog_p);
    return 0;
  }
}

void emit_ins(char ins, FILE *out, size_t *prog_p) {
  fputc(ins, out);
  *prog_p += 1;
}

void emit_ins1(char ins, uint8_t arg, FILE *out, size_t *prog_p) {
  fputc(ins, out);
  fputc(arg, out);
  *prog_p += 2;
}

void emit_ins1_1(char ins, uint8_t arg_a, uint8_t arg_b, FILE *out, size_t *prog_p) {
  fputc(ins, out);
  fputc(arg_a, out);
  fputc(arg_b, out);
  *prog_p += 3;
}

void emit_ins2(char ins, uint16_t arg, FILE *out, size_t *prog_p) {
  fputc(ins, out);
#if BIG_ENDIAN
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >>  0) & 0xFF, out);
#else
  fputc((arg >>  0) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
#endif
  *prog_p += 3;
}

void emit_ins4(char ins, uint32_t arg, FILE *out, size_t *prog_p) {
  fputc(ins, out);
#if BIG_ENDIAN
  fputc((arg >> 24) & 0xFF, out);
  fputc((arg >> 16) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >>  0) & 0xFF, out);
#else
  fputc((arg >>  0) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >> 16) & 0xFF, out);
  fputc((arg >> 24) & 0xFF, out);
#endif
  *prog_p += 5;
}

void emit_ins8(char ins, uint64_t arg, FILE *out, size_t *prog_p) {
  fputc(ins, out);
#if BIG_ENDIAN
  fputc((arg >> 56) & 0xFF, out);
  fputc((arg >> 48) & 0xFF, out);
  fputc((arg >> 40) & 0xFF, out);
  fputc((arg >> 32) & 0xFF, out);
  fputc((arg >> 24) & 0xFF, out);
  fputc((arg >> 16) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >>  0) & 0xFF, out);
#else
  fputc((arg >>  0) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >> 16) & 0xFF, out);
  fputc((arg >> 24) & 0xFF, out);
  fputc((arg >> 32) & 0xFF, out);
  fputc((arg >> 40) & 0xFF, out);
  fputc((arg >> 48) & 0xFF, out);
  fputc((arg >> 56) & 0xFF, out);
#endif
  *prog_p += 9;
}

uint8_t read_int1(char *str) {
  if (*str == '-') {
    int8_t i = atol(str);
    return *((uint8_t *)&i);
  }
  return atol(str);
}

uint16_t read_int2(char *str) {
  if (*str == '-') {
    int16_t i = atol(str);
    return *((uint16_t *)&i);
  }
  return atol(str);
}

uint32_t read_int4(char *str) {
  if (*str == '-') {
    int32_t i = atol(str);
    return *((uint32_t *)&i);
  }
  return atol(str);
}

uint64_t read_int8(char *str) {
  if (*str == '-') {
    int64_t i = atol(str);
    return *((uint64_t *)&i);
  }
  return atol(str);
}

uint32_t read_float4(char *str) {
  float f = atof(str);
  return *((uint32_t *)&f);
}

uint64_t read_float8(char *str) {
  double f = atof(str);
  return *((uint64_t *)&f);
}

void read_operator(uint8_t instruction, FILE *in, FILE *out, size_t *prog_p) {
  char *arg = read_token(in);
  if (strcmp(arg, "dup") == 0) {
    emit_ins1(instruction, OPR_DUP, out, prog_p);
  } else if (strcmp(arg, "neg") == 0) {
    emit_ins1(instruction, OPR_NEG, out, prog_p);
  } else if (strcmp(arg, "add") == 0) {
    emit_ins1(instruction, OPR_ADD, out, prog_p);
  } else if (strcmp(arg, "sub") == 0) {
    emit_ins1(instruction, OPR_SUB, out, prog_p);
  } else if (strcmp(arg, "mul") == 0) {
    emit_ins1(instruction, OPR_MUL, out, prog_p);
  } else if (strcmp(arg, "div") == 0) {
    emit_ins1(instruction, OPR_DIV, out, prog_p);
  } else if (strcmp(arg, "mod") == 0) {
    emit_ins1(instruction, OPR_MOD, out, prog_p);
  } else if (strcmp(arg, "eql") == 0) {
    emit_ins1(instruction, OPR_EQL, out, prog_p);
  } else if (strcmp(arg, "neq") == 0) {
    emit_ins1(instruction, OPR_NEQ, out, prog_p);
  } else if (strcmp(arg, "lth") == 0) {
    emit_ins1(instruction, OPR_LTH, out, prog_p);
  } else if (strcmp(arg, "leq") == 0) {
    emit_ins1(instruction, OPR_LEQ, out, prog_p);
  } else if (strcmp(arg, "gth") == 0) {
    emit_ins1(instruction, OPR_GTH, out, prog_p);
  } else if (strcmp(arg, "geq") == 0) {
    emit_ins1(instruction, OPR_GEQ, out, prog_p);
  } else if (strcmp(arg, "con") == 0) {
    emit_ins1(instruction, OPR_CON, out, prog_p);
  } else if (strcmp(arg, "chd") == 0) {
    emit_ins1(instruction, OPR_CHD, out, prog_p);
  } else if (strcmp(arg, "ctl") == 0) {
    emit_ins1(instruction, OPR_CTL, out, prog_p);
  } else if (strcmp(arg, "arr") == 0) {
    emit_ins1(instruction, OPR_ARR, out, prog_p);
  } else if (strcmp(arg, "agt") == 0) {
    emit_ins1(instruction, OPR_AGT, out, prog_p);
  } else if (strcmp(arg, "ast") == 0) {
    emit_ins1(instruction, OPR_AST, out, prog_p);
  } else if (strcmp(arg, "ci1") == 0) {
    emit_ins1(instruction, OPR_CI1, out, prog_p);
  } else if (strcmp(arg, "ci2") == 0) {
    emit_ins1(instruction, OPR_CI2, out, prog_p);
  } else if (strcmp(arg, "ci4") == 0) {
    emit_ins1(instruction, OPR_CI4, out, prog_p);
  } else if (strcmp(arg, "ci8") == 0) {
    emit_ins1(instruction, OPR_CI8, out, prog_p);
  } else if (strcmp(arg, "cu1") == 0) {
    emit_ins1(instruction, OPR_CU1, out, prog_p);
  } else if (strcmp(arg, "cu2") == 0) {
    emit_ins1(instruction, OPR_CU2, out, prog_p);
  } else if (strcmp(arg, "cu4") == 0) {
    emit_ins1(instruction, OPR_CU4, out, prog_p);
  } else if (strcmp(arg, "cu8") == 0) {
    emit_ins1(instruction, OPR_CU8, out, prog_p);
  } else if (strcmp(arg, "cf4") == 0) {
    emit_ins1(instruction, OPR_CF4, out, prog_p);
  } else if (strcmp(arg, "cf8") == 0) {
    emit_ins1(instruction, OPR_CF8, out, prog_p);
  } else {
    printf("undefined op: %s\n", arg);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    printf("usage: %s IN OUT\n", argv[0]);
    return 1;
  }
  FILE *in = fopen(argv[1], "r");
  FILE *out = fopen(argv[2], "w");
  struct label *labels = NULL;
  size_t prog_p = 0;
  skip_ws(in);
  while (1) {
    char *token = read_token(in);
    if (token == NULL) {
      break;
    }
    skip_ws(in);
    int c = fgetc(in);
    if (c == ':') {
      labels = push_label(labels, token, prog_p);
      skip_ws(in);
      continue;
    } else {
      ungetc(c, in);
    }
    if (strcmp(token, "noop") == 0) {
      emit_ins(INS_NOOP, out, &prog_p);
    } else if (strcmp(token, "exit") == 0) {
      emit_ins(INS_EXIT, out, &prog_p);
    } else if (strcmp(token, "push_nil") == 0) {
      emit_ins(INS_PSHN, out, &prog_p);
    } else if (strcmp(token, "push_sym") == 0) {
      emit_ins(INS_PSHS, out, &prog_p);
      char *arg = read_token(in);
      for (char *c = arg; *c; c++) {
        fputc(*c, out);
        prog_p += 1;
      }
      fputc(0, out);
      prog_p += 1;
      free(arg);
    } else if (strcmp(token, "push_i8") == 0 || strcmp(token, "push_u8") == 0) {
      char *arg = read_token(in);
      emit_ins1(INS_PSH1, read_int1(arg), out, &prog_p);
    } else if (strcmp(token, "push_i16") == 0 || strcmp(token, "push_u16") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_PSH2, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "push_i32") == 0 || strcmp(token, "push_u32") == 0) {
      char *arg = read_token(in);
      emit_ins4(INS_PSH4, read_int4(arg), out, &prog_p);
    } else if (strcmp(token, "push_i64") == 0 || strcmp(token, "push_u64") == 0) {
      char *arg = read_token(in);
      emit_ins8(INS_PSH8, read_int8(arg), out, &prog_p);
    } else if (strcmp(token, "push_f32") == 0) {
      char *arg = read_token(in);
      emit_ins4(INS_PSH4, read_float4(arg), out, &prog_p);
    } else if (strcmp(token, "push_f64") == 0) {
      char *arg = read_token(in);
      emit_ins8(INS_PSH8, read_float8(arg), out, &prog_p);
    } else if (strcmp(token, "push_p") == 0) {
      char *arg = read_token(in);
      size_t addr = get_or_add_label(&labels, arg, prog_p + 1);
      emit_ins4(INS_PSH4, addr, out, &prog_p);
    } else if (strcmp(token, "lod1") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_LOD1, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "lod2") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_LOD2, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "lod4") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_LOD4, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "lod8") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_LOD8, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "lodv") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_LODV, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "sto1") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_STO1, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "sto2") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_STO2, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "sto4") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_STO4, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "sto8") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_STO8, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "stov") == 0) {
      char *arg = read_token(in);
      emit_ins2(INS_STOV, read_int2(arg), out, &prog_p);
    } else if (strcmp(token, "op_i8") == 0) {
      read_operator(INS_OPI1, in, out, &prog_p);
    } else if (strcmp(token, "op_i16") == 0) {
      read_operator(INS_OPI2, in, out, &prog_p);
    } else if (strcmp(token, "op_i32") == 0) {
      read_operator(INS_OPI4, in, out, &prog_p);
    } else if (strcmp(token, "op_i64") == 0) {
      read_operator(INS_OPI8, in, out, &prog_p);
    } else if (strcmp(token, "op_u8") == 0) {
      read_operator(INS_OPU1, in, out, &prog_p);
    } else if (strcmp(token, "op_u16") == 0) {
      read_operator(INS_OPU2, in, out, &prog_p);
    } else if (strcmp(token, "op_u32") == 0) {
      read_operator(INS_OPU4, in, out, &prog_p);
    } else if (strcmp(token, "op_u64") == 0) {
      read_operator(INS_OPU8, in, out, &prog_p);
    } else if (strcmp(token, "op_f32") == 0) {
      read_operator(INS_OPF4, in, out, &prog_p);
    } else if (strcmp(token, "op_f64") == 0) {
      read_operator(INS_OPF8, in, out, &prog_p);
    } else if (strcmp(token, "op_val") == 0) {
      read_operator(INS_OPVA, in, out, &prog_p);
    } else if (strcmp(token, "jump") == 0) {
      char *arg = read_token(in);
      size_t addr = get_or_add_label(&labels, arg, prog_p + 1);
      emit_ins4(INS_JUMP, addr, out, &prog_p);
    } else if (strcmp(token, "jmpc") == 0) {
      char *arg = read_token(in);
      size_t addr = get_or_add_label(&labels, arg, prog_p + 1);
      emit_ins4(INS_JMPC, addr, out, &prog_p);
    } else if (strcmp(token, "call") == 0) {
      uint8_t psize = atoi(read_token(in));
      skip_ws(in);
      uint8_t rsize = atoi(read_token(in));
      emit_ins1_1(INS_CALL, psize, rsize, out, &prog_p);
    } else if (strcmp(token, "retp") == 0) {
      char *arg = read_token(in);
      uint8_t size = atoi(arg);
      emit_ins1(INS_RETP, size, out, &prog_p);
    } else if (strcmp(token, "retr") == 0) {
      emit_ins(INS_RETR, out, &prog_p);
    }
    skip_ws(in);
  }
  while (labels) {
    struct label *l = labels;
    if (!l->seen) {
      printf("undefined label: %s\n", l->name);
    } else {
      while (l->pending) {
        struct offset *off = l->pending;
        printf("write pending label %s:%zd at %zd\n", l->name, l->target, off->offset);
        fseek(out, off->offset, SEEK_SET);
        fputc((l->target >>  0) & 0xFF, out);
        fputc((l->target >>  8) & 0xFF, out);
        fputc((l->target >> 16) & 0xFF, out);
        fputc((l->target >> 24) & 0xFF, out);
        l->pending = off->next;
        free(off);
      }
    }
    labels = l->next;
    free(l);
  }
  fclose(in);
  fclose(out);
}
