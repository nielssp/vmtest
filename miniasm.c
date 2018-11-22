#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "ins.h"

#define BIG_ENDIAN 0

typedef enum {
  MOVE_F64_C_S,
  MOVE_U64_C_S,
  GEQ_U64_O_C_S,
  CONVU64_F64_O_S,
  ADD_F64_S_O_O,
  ADD_U64_O_C_O,
  JUMP_S,
  JUMP,
  EXIT
} Opcode;

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

void emit1(uint8_t arg, FILE *out, size_t *prog_p) {
  printf("%04x: emit %02x\n", *prog_p, arg);
  fputc(arg, out);
  *prog_p += 1;
}

void emit2(uint16_t arg, FILE *out, size_t *prog_p) {
  printf("%04x: emit %04x\n", *prog_p, arg);
#if BIG_ENDIAN
  fputc((arg >>  8) & 0xFF, out);
  fputc((arg >>  0) & 0xFF, out);
#else
  fputc((arg >>  0) & 0xFF, out);
  fputc((arg >>  8) & 0xFF, out);
#endif
  *prog_p += 2;
}

void emit4(uint32_t arg, FILE *out, size_t *prog_p) {
  printf("%04x: emit %08x\n", *prog_p, arg);
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
  *prog_p += 4;
}

void emit8(uint64_t arg, FILE *out, size_t *prog_p) {
  printf("%04x: emit %016x\n", *prog_p, arg);
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
  *prog_p += 8;
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
    if (strcmp(token, "exit") == 0) {
      emit1(EXIT, out, &prog_p);
    } else if (strcmp(token, "move_f64_c_s") == 0) {
      emit1(MOVE_F64_C_S, out, &prog_p);
      emit8(read_float8(read_token(in)), out, &prog_p);
    } else if (strcmp(token, "move_u64_c_s") == 0) {
      emit1(MOVE_U64_C_S, out, &prog_p);
      emit8(read_int8(read_token(in)), out, &prog_p);
    } else if (strcmp(token, "geq_u64_o_c_s") == 0) {
      char *src1 = read_token(in);
      skip_ws(in);
      char *src2 = read_token(in);
      emit1(GEQ_U64_O_C_S, out, &prog_p);
      emit2(read_int2(src1), out, &prog_p);
      emit8(read_int8(src2), out, &prog_p);
    } else if (strcmp(token, "convu64_f64_o_s") == 0) {
      char *src1 = read_token(in);
      emit1(CONVU64_F64_O_S, out, &prog_p);
      emit2(read_int2(src1), out, &prog_p);
    } else if (strcmp(token, "add_f64_s_o_o") == 0) {
      char *src1 = read_token(in);
      skip_ws(in);
      char *dest = read_token(in);
      emit1(ADD_F64_S_O_O, out, &prog_p);
      emit2(read_int2(dest), out, &prog_p);
      emit2(read_int2(src1), out, &prog_p);
    } else if (strcmp(token, "add_u64_o_c_o") == 0) {
      char *src1 = read_token(in);
      skip_ws(in);
      char *src2 = read_token(in);
      skip_ws(in);
      char *dest = read_token(in);
      emit1(ADD_U64_O_C_O, out, &prog_p);
      emit2(read_int2(dest), out, &prog_p);
      emit2(read_int2(src1), out, &prog_p);
      emit8(read_int8(src2), out, &prog_p);
    } else if (strcmp(token, "jump_s") == 0) {
      char *arg = read_token(in);
      size_t addr = get_or_add_label(&labels, arg, prog_p + 1);
      emit1(JUMP_S, out, &prog_p);
      emit2(addr, out, &prog_p);
    } else if (strcmp(token, "jump") == 0) {
      char *arg = read_token(in);
      size_t addr = get_or_add_label(&labels, arg, prog_p + 1);
      emit1(JUMP, out, &prog_p);
      emit2(addr, out, &prog_p);
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
        printf("write pending label %s:%04zx at %04zx\n", l->name, l->target, off->offset);
        fseek(out, off->offset, SEEK_SET);
        fputc((l->target >>  0) & 0xFF, out);
        fputc((l->target >>  8) & 0xFF, out);
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
