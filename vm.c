#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ins.h"
#include "hashmap.h"

#define DEBUG 1

#if DEBUG
#define DBGPRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define DBGPRINT(fmt, ...)
#endif

#define Address uint32_t

#define EXECUTE_INS(ins) \
  DBGPRINT("%ld: ex(%d): ", pc - code, *pc);\
  switch(ins) {\
    case INS_NOOP: goto ins_noop;\
    case INS_EXIT: goto ins_exit;\
    case INS_PSHN: goto ins_pshn;\
    case INS_PSHS: goto ins_pshs;\
    case INS_PSH1: goto ins_psh1;\
    case INS_PSH2: goto ins_psh2;\
    case INS_PSH4: goto ins_psh4;\
    case INS_PSH8: goto ins_psh8;\
    case INS_LOD1: goto ins_lod1;\
    case INS_LOD2: goto ins_lod2;\
    case INS_LOD4: goto ins_lod4;\
    case INS_LOD8: goto ins_lod8;\
    case INS_STO1: goto ins_sto1;\
    case INS_STO2: goto ins_sto2;\
    case INS_STO4: goto ins_sto4;\
    case INS_STO8: goto ins_sto8;\
    case INS_OPI1: goto ins_opi1;\
    case INS_OPI2: goto ins_opi2;\
    case INS_OPI4: goto ins_opi4;\
    case INS_OPI8: goto ins_opi8;\
    case INS_OPU1: goto ins_opu1;\
    case INS_OPU2: goto ins_opu2;\
    case INS_OPU4: goto ins_opu4;\
    case INS_OPU8: goto ins_opu8;\
    case INS_OPF4: goto ins_opf4;\
    case INS_OPF8: goto ins_opf8;\
    case INS_INSP: goto ins_insp;\
    case INS_JUMP: goto ins_jump;\
    case INS_JMPC: goto ins_jmpc;\
    case INS_CALL: goto ins_call;\
    case INS_CRET: goto ins_cret;\
    case INS_SYSC: goto ins_sysc;\
    case INS_OPVA: goto ins_opva;\
  }

#define EXECUTE_OP(label_suffix, op) switch(op) {\
  case OPR_NEG: goto opr_neg_ ## label_suffix;\
  case OPR_ADD: goto opr_add_ ## label_suffix;\
  case OPR_SUB: goto opr_sub_ ## label_suffix;\
  case OPR_MUL: goto opr_mul_ ## label_suffix;\
  case OPR_DIV: goto opr_div_ ## label_suffix;\
  case OPR_MOD: goto opr_mod_ ## label_suffix;\
  case OPR_EQL: goto opr_eql_ ## label_suffix;\
  case OPR_NEQ: goto opr_neq_ ## label_suffix;\
  case OPR_LTH: goto opr_lth_ ## label_suffix;\
  case OPR_LEQ: goto opr_leq_ ## label_suffix;\
  case OPR_GTH: goto opr_gth_ ## label_suffix;\
  case OPR_GEQ: goto opr_geq_ ## label_suffix;\
  case OPR_CON: goto opr_con_ ## label_suffix;\
  case OPR_CHD: goto opr_chd_ ## label_suffix;\
  case OPR_CTL: goto opr_ctl_ ## label_suffix;\
  case OPR_ARR: goto opr_arr_ ## label_suffix;\
  case OPR_AGT: goto opr_agt_ ## label_suffix;\
  case OPR_AST: goto opr_ast_ ## label_suffix;\
  case OPR_CI1: goto opr_ci1_ ## label_suffix;\
  case OPR_CI2: goto opr_ci2_ ## label_suffix;\
  case OPR_CI4: goto opr_ci4_ ## label_suffix;\
  case OPR_CI8: goto opr_ci8_ ## label_suffix;\
  case OPR_CU1: goto opr_cu1_ ## label_suffix;\
  case OPR_CU2: goto opr_cu2_ ## label_suffix;\
  case OPR_CU4: goto opr_cu4_ ## label_suffix;\
  case OPR_CU8: goto opr_cu8_ ## label_suffix;\
  case OPR_CF4: goto opr_cf4_ ## label_suffix;\
  case OPR_CF8: goto opr_cf8_ ## label_suffix;\
  case OPR_DUP: goto opr_dup_ ## label_suffix;\
}

#define PUSH_INSTRUCTION(type) \
  *((type *)sp) = *((type *)pc);\
  DBGPRINT("push: %d to %ld\n", *((type *)sp), sp - stack);\
  sp += sizeof(type);\
  pc += sizeof(type);\
  EXECUTE_INS(*(pc++));

#define LOAD_INSTRUCTION(type) \
  ar = *((int16_t *)pc);\
  *((type *)sp) = *((type *)(bp + ar));\
  DBGPRINT("load: %ld from %d\n", *((type *)sp), bp - stack + ar);\
  sp += sizeof(type);\
  pc += 2;\
  EXECUTE_INS(*(pc++));

#define STORE_INSTRUCTION(type) \
  ar = *((int16_t *)pc);\
  sp -= sizeof(type);\
  *((type *)(bp + ar)) = *((type *)sp);\
  pc += 2;\
  DBGPRINT("store: %ld to %ld\n", *((type *)sp), bp - stack + ar);\
  EXECUTE_INS(*(pc++));

#define BIN_ARITH_OPERATOR(type, operator) \
  sp -= sizeof(type);\
  DBGPRINT("%ld " #operator " %ld", *((type *)(sp - sizeof(type))), *((type *)sp));\
  *((type *)(sp - sizeof(type))) = *((type *)(sp - sizeof(type))) operator *((type *)sp);\
  DBGPRINT(" = %ld\n", *((type *)(sp - sizeof(type))));\
  EXECUTE_INS(*(pc++));

#define BIN_LOGIC_OPERATOR(type, operator) \
  sp -= 2 * sizeof(type);\
  DBGPRINT("%ld " #operator " %ld", *((type *)sp), *((type *)(sp + sizeof(type))));\
  *sp = (uint8_t)(*((type *)sp) operator *((type *)(sp + sizeof(type))));\
  DBGPRINT(" = %d\n", *sp);\
  sp++;\
  EXECUTE_INS(*(pc++));

#define CAST_OPERATOR(type, from_type) \
  sp -= sizeof(from_type);\
  *((type *)sp) = (type)*((from_type *)sp);\
  sp += sizeof(type);\
  EXECUTE_INS(*(pc++));

#define NUMBER_OPERATORS(label_suffix, type, cell_type_const) \
opr_dup_ ## label_suffix:\
  *((type *)sp) = *((type *)(sp - sizeof(type)));\
  sp += sizeof(type);\
  EXECUTE_INS(*(pc++));\
opr_neg_ ## label_suffix:\
  *((type *)(sp - sizeof(type))) = -*((type *)(sp - sizeof(type)));\
  EXECUTE_INS(*(pc++));\
opr_add_ ## label_suffix: BIN_ARITH_OPERATOR(type, +)\
opr_sub_ ## label_suffix: BIN_ARITH_OPERATOR(type, -)\
opr_mul_ ## label_suffix: BIN_ARITH_OPERATOR(type, *)\
opr_div_ ## label_suffix: BIN_ARITH_OPERATOR(type, /)\
opr_eql_ ## label_suffix: BIN_LOGIC_OPERATOR(type, ==)\
opr_neq_ ## label_suffix: BIN_LOGIC_OPERATOR(type, !=)\
opr_lth_ ## label_suffix: BIN_LOGIC_OPERATOR(type, <)\
opr_leq_ ## label_suffix: BIN_LOGIC_OPERATOR(type, <=)\
opr_gth_ ## label_suffix: BIN_LOGIC_OPERATOR(type, >)\
opr_geq_ ## label_suffix: BIN_LOGIC_OPERATOR(type, >=)\
opr_ci1_ ## label_suffix: CAST_OPERATOR(type, int8_t)\
opr_ci2_ ## label_suffix: CAST_OPERATOR(type, int16_t)\
opr_ci4_ ## label_suffix: CAST_OPERATOR(type, int32_t)\
opr_ci8_ ## label_suffix: CAST_OPERATOR(type, int64_t)\
opr_cu1_ ## label_suffix: CAST_OPERATOR(type, uint8_t)\
opr_cu2_ ## label_suffix: CAST_OPERATOR(type, uint16_t)\
opr_cu4_ ## label_suffix: CAST_OPERATOR(type, uint32_t)\
opr_cu8_ ## label_suffix: CAST_OPERATOR(type, uint64_t)\
opr_cf4_ ## label_suffix: CAST_OPERATOR(type, float)\
opr_cf8_ ## label_suffix: CAST_OPERATOR(type, double)\
opr_con_ ## label_suffix:\
  sp -= sizeof(type);\
  temp = *(vs - 1);\
  if (temp.ty ## pe != TYPE_CONS && temp.ty ## pe != TYPE_NIL) {\
    printf("cons: type error (%d)", temp.ty ## pe);\
    return 1;\
  }\
  (vs - 1)->ty ## pe = TYPE_CONS;\
  (vs - 1)->cons = malloc(sizeof(Cons) + sizeof(type));\
  (vs - 1)->cons->refs = 1;\
  (vs - 1)->cons->cell_type = cell_type_const;\
  (vs - 1)->cons->tail = temp.ty ## pe == TYPE_CONS ? temp.cons : NULL;\
  *((type *)(vs - 1)->cons->head) = *((type *)sp);\
  EXECUTE_INS(*(pc++));\
opr_chd_ ## label_suffix:\
  vs--;\
  if (vs->ty ## pe != TYPE_CONS) {\
    printf("head: type error");\
    return 1;\
  }\
  *((type *)sp) = *((type *)vs->cons->head);\
  vs->cons->refs--;\
  if (vs->cons->refs < 1) delete_cons(vs->cons);\
  DBGPRINT("head: %d to %ld\n", *((type *)sp), sp - stack);\
  sp += sizeof(type);\
  EXECUTE_INS(*(pc++));\
opr_ctl_ ## label_suffix:\
  temp = *(vs - 1);\
  if (temp.ty ## pe != TYPE_CONS) {\
    printf("tail: type error");\
    return 1;\
  }\
  if (temp.cons->tail) {\
    (vs - 1)->ty ## pe = TYPE_CONS;\
    (vs - 1)->cons = temp.cons->tail;\
    (vs - 1)->cons->refs++;\
  } else {\
    (vs - 1)->ty ## pe = TYPE_NIL;\
  }\
  temp.cons->refs--;\
  if (temp.cons->refs < 1) delete_cons(temp.cons);\
  EXECUTE_INS(*(pc++));\
opr_arr_ ## label_suffix:\
  sp -= sizeof(uint64_t);\
  vs->ty ## pe = TYPE_ARRAY;\
  vs->array = calloc(sizeof(Array) + sizeof(type) * *((uint64_t *)sp), 1);\
  vs->array->refs = 1;\
  vs->array->length = *((uint64_t *)sp);\
  vs->array->cell_type = cell_type_const;\
  vs++;\
  EXECUTE_INS(*(pc++));\
opr_agt_ ## label_suffix:\
  vs--;\
  if (vs->ty ## pe != TYPE_ARRAY) {\
    printf("get: type error");\
    return 1;\
  }\
  sp -= sizeof(uint64_t);\
  *((type *)sp) = ((type *)vs->array->data)[*((uint64_t *)sp)];\
  vs->array->refs--;\
  if (vs->array->refs < 1) delete_array(vs->array);\
  DBGPRINT("get: %d to %ld\n", *((type *)sp), sp - stack);\
  sp += sizeof(type);\
  EXECUTE_INS(*(pc++));\
opr_ast_ ## label_suffix:\
  vs--;\
  if (vs->ty ## pe != TYPE_ARRAY) {\
    printf("set: type error");\
    return 1;\
  }\
  sp -= sizeof(uint64_t) + sizeof(type);\
  ((type *)vs->array->data)[*((uint64_t *)sp)] = *((type *)(sp + sizeof(uint64_t)));\
  vs->array->refs--;\
  if (vs->array->refs < 1) delete_array(vs->array);\
  DBGPRINT("set: [%ld] to %ld\n", *((uint64_t *)sp), *((type *)(sp + sizeof(uint64_t))));\
  EXECUTE_INS(*(pc++));

#define INTEGER_OPERATORS(label_suffix, type, cell_type_const) DBGPRINT("op(%d)\n", *pc);EXECUTE_OP(label_suffix, *(pc++));\
  NUMBER_OPERATORS(label_suffix, type, cell_type_const)\
opr_mod_ ## label_suffix: BIN_ARITH_OPERATOR(type, %)

#define FLOAT_OPERATORS(label_suffix, type, cell_type_const) EXECUTE_OP(label_suffix, *(pc++));\
  NUMBER_OPERATORS(label_suffix, type, cell_type_const)\
opr_mod_ ## label_suffix:\
  printf("invalid float operation");\
  return 1;

typedef struct val Val;
typedef struct cons Cons;
typedef struct array Array;
typedef struct symbol Symbol;

typedef enum {
  TYPE_U8,
  TYPE_U16,
  TYPE_U32,
  TYPE_U64,
  TYPE_I8,
  TYPE_I16,
  TYPE_I32,
  TYPE_I64,
  TYPE_F32,
  TYPE_F64,
  TYPE_VAL
} CellType;

typedef enum {
  TYPE_UNDEFINED,
  TYPE_NIL,
  TYPE_CONS,
  TYPE_ARRAY,
  TYPE_SYMBOL
} ValType;

struct val {
  ValType type;
  union {
    Cons *cons;
    Array *array;
    Symbol *symbol;
  };
};

struct cons {
  uint64_t refs;
  Cons *tail;
  CellType cell_type;
  uint8_t head[];
};

struct array {
  uint64_t refs;
  uint64_t length;
  CellType cell_type;
  uint8_t data[];
};

struct symbol {
  uint64_t refs;
  uint64_t length;
  char symbol[];
};

struct process {
  size_t p_size;
  uint8_t *p_stack;
  size_t v_size;
  Val *v_stack;
  char *pc;
  uint8_t *ps;
  uint8_t *pb;
  uint8_t *vs;
  uint8_t *vb;
};

size_t code_size;
char *code = NULL;

size_t stack_size = 1024;
uint8_t *stack = NULL;
Val *val_stack = NULL;

Val temp = { .type = TYPE_UNDEFINED };

char *pc = NULL; /* program counter */
uint8_t *sp = NULL; /* stack pointer */
uint8_t *bp = NULL; /* base pointer */
Val *vs = NULL; /* value stack pointer */
Val *vb = NULL; /* value base pointer */
int16_t ar = 0; /* arg */
Address ad = 0; /* addr */

void dump_stack() {
  return;
  for (uint8_t *s = stack; s != sp; s++) {
    printf("%ld: %x     [%d, %d, %d, %ld]\n", s - stack, *s, *s, *((int16_t *)s), *((int32_t *)s), *((int64_t *)s));
  }
}

void delete_array(Array *array);
void delete_cons(Cons *cons);

DEFINE_PRIVATE_HASH_MAP(sym_map, SymMap, char *, Symbol *, string_hash, string_equals);

SymMap symbols = (SymMap){ .map = NULL };

Val create_string(const char *str) {
  uint64_t length = strlen(str);
  Array *array = malloc(sizeof(Array) + length + 1);
  array->refs = 1;
  array->length = length;
  array->cell_type = TYPE_U8;
  strcpy(array->data, str);
  return (Val){ .type = TYPE_ARRAY, .array = array };
}

Val create_array(uint64_t length) {
  Array *array = calloc(sizeof(Array) + sizeof(Val) * length, 1);
  array->refs = 1;
  array->length = length;
  array->cell_type = TYPE_VAL;
  return (Val){ .type = TYPE_ARRAY, .array = array };
}

Val create_symbol(const char *str) {
  if (!symbols.map) {
    symbols = create_sym_map();
  }
  Symbol *sym = sym_map_lookup(symbols, str);
  if (sym) {
    sym->refs++;
    return (Val){ .type = TYPE_SYMBOL, .symbol = sym };
  }
  size_t length = strlen(str);
  Symbol *symbol = malloc(sizeof(Symbol) + length + 1);
  symbol->refs = 2;
  symbol->length = length;
  strcpy(symbol->symbol, str);
  sym_map_add(symbols, symbol->symbol, symbol);
  return (Val){ .type = TYPE_SYMBOL, .symbol = symbol };
}

void copy_val(Val val) {
  switch (val.type) {
    case TYPE_ARRAY:
      val.array->refs++;
      break;
    case TYPE_CONS:
      val.cons->refs++;
      break;
    case TYPE_SYMBOL:
      val.symbol->refs++;
      break;
  }
}

void delete_val(Val val) {
  switch (val.type) {
    case TYPE_ARRAY:
      val.array->refs--;
      if (val.array->refs < 1) delete_array(val.array);
      break;
    case TYPE_CONS:
      val.cons->refs--;
      if (val.cons->refs < 1) delete_cons(val.cons);
      break;
    case TYPE_SYMBOL:
      val.symbol->refs--;
      if (val.symbol->refs < 1) free(val.symbol);
      break;
  }
}

void delete_array(Array *array) {
  DBGPRINT("delete array %zx\n", array);
  if (array->cell_type == TYPE_VAL) {
    for (uint64_t i = 0; i < array->length; i++) {
      delete_val(((Val *)array->data)[i]);
    }
  }
  free(array);
}

void delete_cons(Cons *cons) {
  Cons *tail = cons->tail;
  DBGPRINT("delete cons %zx\n", cons);
  if (cons->cell_type == TYPE_VAL) delete_val(*((Val *)cons->head));
  free(cons);
  while (tail) {
    tail->refs--;
    if (tail->refs > 0) {
      return;
    }
    cons = tail;
    tail = tail->tail;
    DBGPRINT("delete cons %zx\n", cons);
    if (cons->cell_type == TYPE_VAL) delete_val(*((Val *)cons->head));
    free(cons);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("usage: %s PROGRAM\n", argv[0]);
    return 1;
  }
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    printf("usage: program fopen error\n");
    return 1;
  }
  fseek(f, 0L, SEEK_END);
  code_size = ftell(f);
  code = malloc(code_size);
  fseek(f, 0L, SEEK_SET);
  fread(code, 1, code_size, f);
  fclose(f);

  stack = calloc(stack_size, sizeof(uint8_t));
  sp = stack;
  bp = stack;

  val_stack = calloc(stack_size, sizeof(Val));
  vs = val_stack;
  vb = val_stack;

  pc = code;

  char *command = malloc(strlen(argv[0]) + strlen(argv[1]) + 2);
  strcpy(command, argv[0]);
  strcat(command, argv[1]);
  Val args = create_array(argc - 1);
  ((Val *)args.array->data)[0] = create_string(command);
  free(command);
  for (int i = 1; i < args.array->length; i++) {
    ((Val *)args.array->data)[i] = create_string(argv[i + 1]);
  }
  *(vs++) = args;

ins_noop: EXECUTE_INS(*(pc++));
ins_exit:
  DBGPRINT("exit\n");
  return 0;
ins_pshn:
  DBGPRINT("push: NIL to %ld\n", vs - val_stack);
  (vs++)->type = TYPE_NIL;
  EXECUTE_INS(*(pc++));
ins_pshs:
  *vs = create_symbol((char *)pc);
  DBGPRINT("push: %s to %ld\n", vs->symbol->symbol, vs - val_stack);
  pc += vs->symbol->length + 1;
  vs++;
  EXECUTE_INS(*(pc++));
ins_psh1: PUSH_INSTRUCTION(uint8_t);
ins_psh2: PUSH_INSTRUCTION(uint16_t);
ins_psh4: PUSH_INSTRUCTION(uint32_t);
ins_psh8: PUSH_INSTRUCTION(uint64_t);
ins_lod1: LOAD_INSTRUCTION(uint8_t);
ins_lod2: LOAD_INSTRUCTION(uint16_t);
ins_lod4: LOAD_INSTRUCTION(uint32_t);
ins_lod8: LOAD_INSTRUCTION(uint64_t);
ins_sto1: STORE_INSTRUCTION(uint8_t);
ins_sto2: STORE_INSTRUCTION(uint16_t);
ins_sto4: STORE_INSTRUCTION(uint32_t);
ins_sto8: STORE_INSTRUCTION(uint64_t);
ins_opi1: INTEGER_OPERATORS(i1, int8_t, TYPE_I8);
ins_opi2: INTEGER_OPERATORS(i2, int16_t, TYPE_I16);
ins_opi4: INTEGER_OPERATORS(i4, int32_t, TYPE_I32);
ins_opi8: INTEGER_OPERATORS(i8, int64_t, TYPE_I64);
ins_opu1: INTEGER_OPERATORS(u1, uint8_t, TYPE_U8); 
ins_opu2: INTEGER_OPERATORS(u2, uint16_t, TYPE_U16);
ins_opu4: INTEGER_OPERATORS(u4, uint32_t, TYPE_U32); 
ins_opu8: INTEGER_OPERATORS(u8, uint64_t, TYPE_U64);
ins_opf4: FLOAT_OPERATORS(f4, float, TYPE_F32);
ins_opf8: FLOAT_OPERATORS(f8, double, TYPE_F64);
ins_opva:
  switch (*(pc++)) {
    case OPR_DUP:
      copy_val(*(vs - 1));
      *(vs++) = *(vs - 1);
      EXECUTE_INS(*(pc++));
  }
  EXECUTE_INS(*(pc++));
ins_insp:
  printf("not implemented\n");
  return 0;
ins_jump:
  DBGPRINT("jump: (addr: %d)\n", *((Address *)pc));
  pc = code + *((Address *)pc);
  EXECUTE_INS(*(pc++));

ins_jmpc:
  DBGPRINT("cond jump: %d (addr: %d)\n", *(sp - 1), *((Address *)pc));
  if (*(--sp)) {
    pc = code + *((Address *)pc);
  } else {
    pc += sizeof(Address);
  }
  EXECUTE_INS(*(pc++));

ins_call:
  sp -= sizeof(Address);
  ad = *((Address *)sp);
  DBGPRINT("call %ld (sp:%ld)\n", ad, sp - stack);
  *((uint8_t **)sp) = bp;
  bp = sp;
  sp += sizeof(uint8_t *);
  *((uint8_t **)sp) = pc;
  sp += sizeof(uint8_t *);
  pc = code + ad;
  dump_stack();
  EXECUTE_INS(*(pc++));

ins_cret:
  DBGPRINT("ret (bp:%ld, addr:%d)\n", bp - stack, *((int32_t *)pc));
  dump_stack();
  sp = bp + *((int32_t *)pc);
  DBGPRINT("restore sp: %ld\n", sp - stack);
  dump_stack();
  pc = *((uint8_t **)(bp + sizeof(uint8_t *)));
  DBGPRINT("restore pc: %ld\n", pc - code);
  bp = *((uint8_t **)bp);
  EXECUTE_INS(*(pc++));

ins_sysc:
  printf("not implemented\n");
  EXECUTE_INS(*(pc++));

  return 0;
}
