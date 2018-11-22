#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "ins.h"
#include "hashmap.h"

#define DEBUG 0

#if DEBUG
#define DBGPRINT(...) printf(__VA_ARGS__)
#define DBGPRINT_REF(ref) print_ref(ref)
#else
#define DBGPRINT(...)
#define DBGPRINT_REF(ref)
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
    case INS_LODV: goto ins_lodv;\
    case INS_STO1: goto ins_sto1;\
    case INS_STO2: goto ins_sto2;\
    case INS_STO4: goto ins_sto4;\
    case INS_STO8: goto ins_sto8;\
    case INS_STOV: goto ins_stov;\
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
    case INS_RETP: goto ins_retp;\
    case INS_RETR: goto ins_retr;\
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
  DBGPRINT("push: %lf to %ld\n", (double)*((type *)sp), sp - stack);\
  sp += sizeof(type);\
  pc += sizeof(type);\
  EXECUTE_INS(*(pc++));

#define LOAD_INSTRUCTION(type) \
  ar = *((int16_t *)pc);\
  *((type *)sp) = *((type *)(bp + ar));\
  DBGPRINT("load: %lf from %ld\n", (double)*((type *)sp), bp - stack + ar);\
  sp += sizeof(type);\
  pc += 2;\
  EXECUTE_INS(*(pc++));

#define STORE_INSTRUCTION(type) \
  ar = *((int16_t *)pc);\
  sp -= sizeof(type);\
  *((type *)(bp + ar)) = *((type *)sp);\
  pc += 2;\
  DBGPRINT("store: %lf to %ld\n", (double)*((type *)sp), bp - stack + ar);\
  EXECUTE_INS(*(pc++));

#define BIN_ARITH_OPERATOR(type, operator) \
  sp -= sizeof(type);\
  DBGPRINT("%f %s %f", (double)*((type *)(sp - sizeof(type))), #operator, (double)*((type *)sp));\
  *((type *)(sp - sizeof(type))) = *((type *)(sp - sizeof(type))) operator *((type *)sp);\
  DBGPRINT(" = %lf\n", (double)*((type *)(sp - sizeof(type))));\
  EXECUTE_INS(*(pc++));

#define BIN_LOGIC_OPERATOR(type, operator) \
  sp -= 2 * sizeof(type);\
  DBGPRINT("%f %s %f", (double)*((type *)sp), #operator, (double)*((type *)(sp + sizeof(type))));\
  *sp = (uint8_t)(*((type *)sp) operator *((type *)(sp + sizeof(type))));\
  DBGPRINT(" = %lf\n", (double)*sp);\
  sp++;\
  EXECUTE_INS(*(pc++));

#define CAST_OPERATOR(type, from_type) \
  sp -= sizeof(from_type);\
  DBGPRINT("cast: %lf to %lf\n", (double)*((from_type *)sp), (double)(type)*((from_type *)sp));\
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
  DBGPRINT("head: %lf to %ld\n", (double)*((type *)sp), sp - stack);\
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
  DBGPRINT("get: %lf to %ld\n", (double)*((type *)sp), sp - stack);\
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
  DBGPRINT("set: [%ld] to %lf\n", (long)*((uint64_t *)sp), (double)*((type *)(sp + sizeof(uint64_t))));\
  EXECUTE_INS(*(pc++));

#define INTEGER_OPERATORS(label_suffix, type, cell_type_const) DBGPRINT("op(%d)\n", *pc);EXECUTE_OP(label_suffix, *(pc++));\
  NUMBER_OPERATORS(label_suffix, type, cell_type_const)\
opr_mod_ ## label_suffix: BIN_ARITH_OPERATOR(type, %)

#define FLOAT_OPERATORS(label_suffix, type, cell_type_const) EXECUTE_OP(label_suffix, *(pc++));\
  NUMBER_OPERATORS(label_suffix, type, cell_type_const)\
opr_mod_ ## label_suffix:\
  printf("invalid float operation");\
  return 1;

typedef struct ref Ref;
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
  TYPE_REF
} CellType;

typedef enum {
  TYPE_UNDEFINED,
  TYPE_NIL,
  TYPE_CONS,
  TYPE_ARRAY,
  TYPE_SYMBOL
} RefType;

struct ref {
  RefType type;
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

struct slice {
  uint64_t refs;
  uint64_t length;
  CellType cell_type;
  Array *source;
  uint8_t *data;
};

struct symbol {
  uint64_t refs;
  uint64_t length;
  char symbol[];
};

struct process {
  size_t p_size;
  uint8_t *p_stack;
  size_t r_size;
  Ref *r_stack;
  char *pc;
  uint8_t *ps;
  uint8_t *pb;
  uint8_t *rs;
  uint8_t *rb;
};

size_t code_size;
char *code = NULL;

size_t stack_size = 1024;
uint8_t *stack = NULL;
Ref *ref_stack = NULL;

Ref temp = { .type = TYPE_UNDEFINED };

char *pc = NULL; /* program counter */
uint8_t *sp = NULL; /* stack pointer */
uint8_t *bp = NULL; /* base pointer */
Ref *vs = NULL; /* reference stack pointer */
Ref *vb = NULL; /* reference base pointer */
int16_t ar = 0; /* arg */
Address ad = 0; /* addr */
uint8_t pas = 0; /* prim arg size */
uint8_t ras = 0; /* ref arg size */


uint8_t *temp_bp = NULL;
Ref *temp_vb = NULL;

void dump_stack() {
  return;
  for (uint8_t *s = stack; s != sp; s++) {
    printf("%ld: %x     [%d, %d, %d, %ld]\n", s - stack, *s, *s, *((int16_t *)s), *((int32_t *)s), *((int64_t *)s));
  }
}

void delete_array(Array *array);
void delete_cons(Cons *cons);

DEFINE_PRIVATE_HASH_MAP(sym_map, SymMap, char *, Symbol *, string_hash, string_equals)

SymMap symbols;

Ref create_string(const char *str) {
  uint64_t length = strlen(str);
  Array *array = malloc(sizeof(Array) + length + 1);
  array->refs = 1;
  array->length = length;
  array->cell_type = TYPE_U8;
  strcpy((char *)array->data, str);
  return (Ref){ .type = TYPE_ARRAY, .array = array };
}

Ref create_array(uint64_t length) {
  Array *array = calloc(sizeof(Array) + sizeof(Ref) * length, 1);
  array->refs = 1;
  array->length = length;
  array->cell_type = TYPE_REF;
  return (Ref){ .type = TYPE_ARRAY, .array = array };
}

Ref create_symbol(const char *str) {
  Symbol *sym = sym_map_lookup(symbols, str);
  if (sym) {
    sym->refs++;
    return (Ref){ .type = TYPE_SYMBOL, .symbol = sym };
  }
  size_t length = strlen(str);
  Symbol *symbol = malloc(sizeof(Symbol) + length + 1);
  symbol->refs = 2;
  symbol->length = length;
  strcpy(symbol->symbol, str);
  sym_map_add(symbols, symbol->symbol, symbol);
  return (Ref){ .type = TYPE_SYMBOL, .symbol = symbol };
}

void copy_ref(Ref ref) {
  switch (ref.type) {
    case TYPE_ARRAY:
      ref.array->refs++;
      break;
    case TYPE_CONS:
      ref.cons->refs++;
      break;
    case TYPE_SYMBOL:
      ref.symbol->refs++;
      break;
    default:
      break;
  }
}

void print_ref(Ref ref);

void print_cell(CellType cell_type, uint8_t *data, uint64_t index) {
  switch (cell_type) {
    case TYPE_U8:
      printf("%d", ((uint8_t *)data)[index]);
      break;
    case TYPE_U16:
      printf("%d", ((uint16_t *)data)[index]);
      break;
    case TYPE_U32:
      printf("%d", ((uint32_t *)data)[index]);
      break;
    case TYPE_U64:
      printf("%ld", ((uint64_t *)data)[index]);
      break;
    case TYPE_I8:
      printf("%d", ((int8_t *)data)[index]);
      break;
    case TYPE_I16:
      printf("%d", ((int16_t *)data)[index]);
      break;
    case TYPE_I32:
      printf("%d", ((int32_t *)data)[index]);
      break;
    case TYPE_I64:
      printf("%ld", ((int64_t *)data)[index]);
      break;
    case TYPE_F32:
      printf("%f", ((float *)data)[index]);
      break;
    case TYPE_F64:
      printf("%f", ((double *)data)[index]);
      break;
    case TYPE_REF:
      print_ref(((Ref *)data)[index]);
      break;
  }
}

void print_array(Array *array) {
  printf("[");
    for (uint64_t i = 0; i < array->length; i++) {
      if (i) {
        printf(" ");
      }
      print_cell(array->cell_type, array->data, i);
    }
  printf("]");
}

void print_cons(Cons *cons) {
  Cons *tail = cons->tail;
  printf("(");
  print_cell(cons->cell_type, cons->head, 0);
  while (tail) {
    tail->refs--;
    if (tail->refs > 0) {
      return;
    }
    cons = tail;
    tail = tail->tail;
    print_cell(cons->cell_type, cons->head, 0);
  }
  printf(")");
}

void print_symbol(Symbol *symbol) {
  printf("%s", symbol->symbol);
}

void print_ref(Ref ref) {
  switch (ref.type) {
    case TYPE_UNDEFINED:
      printf("undefined");
      break;
    case TYPE_NIL:
      printf("()");
      break;
    case TYPE_ARRAY:
      print_array(ref.array);
      break;
    case TYPE_CONS:
      print_cons(ref.cons);
      break;
    case TYPE_SYMBOL:
      print_symbol(ref.symbol);
      break;
    default:
      break;
  }
}

void delete_ref(Ref ref) {
  switch (ref.type) {
    case TYPE_ARRAY:
      ref.array->refs--;
      if (ref.array->refs < 1) delete_array(ref.array);
      break;
    case TYPE_CONS:
      ref.cons->refs--;
      if (ref.cons->refs < 1) delete_cons(ref.cons);
      break;
    case TYPE_SYMBOL:
      ref.symbol->refs--;
      if (ref.symbol->refs < 1) free(ref.symbol);
      break;
    default:
      break;
  }
}

void delete_array(Array *array) {
  DBGPRINT("delete array %p\n", (void *)array);
  if (array->cell_type == TYPE_REF) {
    for (uint64_t i = 0; i < array->length; i++) {
      delete_ref(((Ref *)array->data)[i]);
    }
  }
  free(array);
}

void delete_cons(Cons *cons) {
  Cons *tail = cons->tail;
  DBGPRINT("delete cons %p\n", (void *)cons);
  if (cons->cell_type == TYPE_REF) delete_ref(*((Ref *)cons->head));
  free(cons);
  while (tail) {
    tail->refs--;
    if (tail->refs > 0) {
      return;
    }
    cons = tail;
    tail = tail->tail;
    DBGPRINT("delete cons %p\n", (void *)cons);
    if (cons->cell_type == TYPE_REF) delete_ref(*((Ref *)cons->head));
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

  ref_stack = calloc(stack_size, sizeof(Ref));
  vs = ref_stack;
  vb = ref_stack;

  pc = code;

  symbols = create_sym_map();

  char *command = malloc(strlen(argv[0]) + strlen(argv[1]) + 2);
  strcpy(command, argv[0]);
  strcat(command, argv[1]);
  Ref args = create_array(argc - 1);
  ((Ref *)args.array->data)[0] = create_string(command);
  free(command);
  for (int i = 1; i < args.array->length; i++) {
    ((Ref *)args.array->data)[i] = create_string(argv[i + 1]);
  }
  *(vs++) = args;
  DBGPRINT("args loaded: ");
  DBGPRINT_REF(*(vs - 1));
  DBGPRINT("\n");

ins_noop: EXECUTE_INS(*(pc++));
ins_exit:
  DBGPRINT("exit\n");
  return 0;
ins_pshn:
  DBGPRINT("push: NIL to %ld\n", vs - ref_stack);
  (vs++)->type = TYPE_NIL;
  EXECUTE_INS(*(pc++));
ins_pshs:
  *vs = create_symbol((char *)pc);
  DBGPRINT("push: %s to %ld\n", vs->symbol->symbol, vs - ref_stack);
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
ins_lodv:
  ar = *((int16_t *)pc);
  *vs = *(vb + ar);
  copy_ref(*vs);
  DBGPRINT("load: ");
  DBGPRINT_REF(*vs);
  DBGPRINT(" from %ld\n", vb - ref_stack + ar);
  vs++;
  pc += 2;
  EXECUTE_INS(*(pc++));
ins_sto1: STORE_INSTRUCTION(uint8_t);
ins_sto2: STORE_INSTRUCTION(uint16_t);
ins_sto4: STORE_INSTRUCTION(uint32_t);
ins_sto8: STORE_INSTRUCTION(uint64_t);
ins_stov:
  ar = *((int16_t *)pc);
  vs--;
  delete_ref(*(vb + ar));
  *(vb + ar) = *vs;
  DBGPRINT("store: ");
  DBGPRINT_REF(*vs);
  DBGPRINT(" to %ld\n", vb - ref_stack + ar);
  EXECUTE_INS(*(pc++));
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
      copy_ref(*(vs - 1));
      *vs = *(vs - 1);
      vs++;
      EXECUTE_INS(*(pc++));
    case OPR_AGT:
      temp = *(vs - 1);
      if (temp.type != TYPE_ARRAY) {
        printf("get: type error");
        return 1;
      }
      sp -= sizeof(uint64_t);
      *(vs - 1) = ((Ref *)temp.array->data)[*((uint64_t *)sp)];
      copy_ref(*(vs - 1));
      DBGPRINT("get: ");
      DBGPRINT_REF(*(vs - 1));
      temp.array->refs--;
      if (temp.array->refs < 1) delete_array(temp.array);
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
  pas = *((uint8_t *)(pc++));
  ras = *((uint8_t *)(pc++));
  DBGPRINT("call %d (sp:%ld, pas:%d, ras:%d)\n", ad, sp - stack, pas, ras);
  sp -= pas;
  // move primitive parameters
  memmove(sp + sizeof(char *) + sizeof(Ref *) + sizeof(uint8_t *), sp, pas);
  // push return address
  *((char **)sp) = pc;
  sp += sizeof(char *);
  pc = code + ad;
  // push reference base pointer
  *((Ref **)sp) = vb;
  sp += sizeof(Ref *);
  vb = vs - ras;
  // push primitive base pointer
  *((uint8_t **)sp) = bp;
  sp += sizeof(uint8_t *);
  bp = sp;
  sp += pas;
  dump_stack();
  EXECUTE_INS(*(pc++));

ins_retp:
  dump_stack();
  pas = *((uint8_t *)(pc++));
  DBGPRINT("ret (bp:%ld, pas:%d)\n", bp - stack, pas);
  // restore pc
  pc = *((char **)(bp - sizeof(uint8_t *) - sizeof(Ref *) - sizeof(char *)));
  DBGPRINT("restore pc: %ld\n", pc - code);
  temp_vb = *((Ref **)(bp - sizeof(uint8_t *) - sizeof(Ref *)));
  temp_bp = *((uint8_t **)(bp - sizeof(uint8_t *)));
  // move primitive return value
  memmove(bp - sizeof(uint8_t *) - sizeof(Ref *) - sizeof(char *), sp - pas, pas);
  sp = bp - sizeof(uint8_t *) - sizeof(Ref *) - sizeof(char *) + pas;
  DBGPRINT("restore sp: %ld\n", sp - stack);
  dump_stack();
  while (vs > vb) {
    delete_ref(*(vs--));
  }
  DBGPRINT("restore vs: %ld\n", vs - ref_stack);
  vb = temp_vb;
  bp = temp_bp;
  EXECUTE_INS(*(pc++));

ins_retr:
  dump_stack();
  DBGPRINT("ret (bp:%ld)\n", bp - stack);
  // restore pc
  pc = *((char **)(bp - sizeof(uint8_t *) - sizeof(Ref *) - sizeof(char *)));
  DBGPRINT("restore pc: %ld\n", pc - code);
  // move reference return value
  temp_vb = vs - 1;
  copy_ref(*temp_vb);
  while (vs > vb) {
    delete_ref(*(vs--));
  }
  sp = bp - sizeof(uint8_t *) - sizeof(Ref *) - sizeof(char *);
  DBGPRINT("restore sp: %ld\n", sp - stack);
  dump_stack();
  vs = vb;
  *(vs++) = *temp_vb;
  DBGPRINT("restore vs: %ld\n", vs - ref_stack);
  vb = *((Ref **)(bp - sizeof(uint8_t *) - sizeof(Ref *)));
  bp = *((uint8_t **)(bp - sizeof(uint8_t *)));
  EXECUTE_INS(*(pc++));

ins_sysc:
  printf("not implemented\n");
  EXECUTE_INS(*(pc++));

  return 0;
}
