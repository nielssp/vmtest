#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#define Address uint16_t
#define DEBUG 0

#if DEBUG
#define DBGPRINT(...) printf(__VA_ARGS__)
#else
#define DBGPRINT(...)
#endif

size_t code_size;
char *code = NULL;

size_t stack_size = 1024;
uint8_t *stack = NULL;

char *pc = NULL; /* program counter */
uint8_t *sp = NULL; /* stack pointer */
uint8_t *bp = NULL; /* base pointer */
Address src1 = 0; /* addr */
Address src2 = 0; /* addr */
Address dest = 0; /* addr */

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

  pc = code;

  void *jump[] = { &&move_f64_c_s, &&move_u64_c_s, &&geq_u64_o_c_s, &&convu64_f64_o_s, &&add_f64_s_o_o, &&add_u64_o_c_o, &&jump_s, &&jump, &&exit };

  goto *(jump[*(pc++)]);

move_f64_c_s:
  *((double *)sp) = *((double *)pc);
  sp += sizeof(double);
  pc += sizeof(double);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

move_u64_c_s:
  *((uint64_t *)sp) = *((uint64_t *)pc);
  sp += sizeof(uint64_t);
  pc += sizeof(uint64_t);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

geq_u64_o_c_s:
  src1 = *((Address *)pc);
  pc += sizeof(Address);
  *((uint64_t *)sp) = *((uint64_t *)(bp + src1)) >= *((uint64_t *)pc);
  pc += sizeof(uint64_t);
  sp += sizeof(uint64_t);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

convu64_f64_o_s:
  src1 = *((Address *)pc);
  pc += sizeof(Address);
  *((double *)sp) = (double)*((uint64_t *)(bp + src1));
  sp += sizeof(double);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

add_f64_s_o_o:
  dest = *((Address *)pc);
  pc += sizeof(Address);
  src2 = *((Address *)pc);
  pc += sizeof(Address);
  sp -= sizeof(double);
  *((double *)(bp + dest)) = *((double *)sp) + *((double *)(bp + src2));
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

add_u64_o_c_o:
  dest = *((Address *)pc);
  pc += sizeof(Address);
  src1 = *((Address *)pc);
  pc += sizeof(Address);
  *((uint64_t *)(bp + dest)) = *((uint64_t *)(bp + src1)) + *((uint64_t *)pc);
  pc += sizeof(uint64_t);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

jump_s:
  sp -= sizeof(uint64_t);
  if (*((uint64_t *)sp)) {
    pc = code + *((Address *)pc);
  } else {
    pc += sizeof(Address);
  }
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

jump:
  pc = code + *((Address *)pc);
  DBGPRINT("%04x: ex %d\n", pc - code, *pc);
  goto *(jump[*(pc++)]);

exit:
  return 0;
}
