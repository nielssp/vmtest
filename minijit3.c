#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include "hashmap.h"

#define Address uint16_t
#define DEBUG 0

#if DEBUG
#define DBGPRINT(...) printf(__VA_ARGS__)
#else
#define DBGPRINT(...)
#endif

/* https://nullprogram.com/blog/2015/03/19/ */

struct asmbuf {
  size_t size;
  uint8_t *next;
  uint8_t code[];
};

size_t size_t_hash(const void *p) {
  return (size_t)p;
}

int size_t_equals(const void *a, const void *b) {
  return a == b;
}

DEFINE_PRIVATE_HASH_MAP(offset_map, OffsetMap, size_t, uint8_t *, size_t_hash, size_t_equals);

struct asmbuf *create_asmbuf() {
  int prot = PROT_READ | PROT_WRITE;
  int flags = MAP_ANONYMOUS | MAP_PRIVATE;
  size_t size = sysconf(_SC_PAGESIZE);
  struct asmbuf *buf = mmap(NULL, size, prot, flags, -1, 0);
  buf->size = size - sizeof(struct asmbuf);
  buf->next = buf->code;
  return buf;
}

int finalize_asmbuf(struct asmbuf *buf) {
  return mprotect(buf, sizeof(struct asmbuf) + buf->size, PROT_READ | PROT_EXEC);
}

void delete_asmbuf(struct asmbuf *buf) {
  munmap(buf, sizeof(struct asmbuf) + buf->size);
}

void asmbuf_ins(struct asmbuf *buf, int size, uint64_t ins) {
  while (size > 0) {
    *(buf->next++) = ins >> (--size * 8);
  }
}

void asmbuf_immediate(struct asmbuf *buf, int size, const char *value) {
  memcpy(buf->next, value, size);
  buf->next += size;
}

size_t code_size;
char *code = NULL;

size_t stack_size = 1024;
uint8_t *stack = NULL;
uint8_t *sp = NULL; /* stack pointer */
uint8_t *bp = NULL; /* base pointer */

typedef enum {
  MOVE_F64_C_S,
  MOVE_U64_C_S,
  GEQ_U64_O_C,
  CONVU64_F64_O_S,
  ADD_F64_S_O,
  ADD_U64_C_O,
  CJUMP,
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

  struct asmbuf *buf = create_asmbuf();

  // Reserved registers:
  // rdi: stack frame pointer
  // rsi: top of stack pointer
  // rdx: comparison result
  
  OffsetMap pending_offsets = create_offset_map();
  int32_t *instruction_offsets = malloc(sizeof(uint32_t) * code_size);


  for (int i = 0; i < code_size; i++) {
    Address src, dest;
    int32_t displacement;
    uint8_t *offset = offset_map_remove(pending_offsets, i);
    if (offset) {
      displacement = buf->next - offset;
      printf("write displacement %d to %p\n", displacement, offset - 4);
      memcpy(offset - 4, (char *)&displacement, 4);
    }
    offset = buf->next;
    instruction_offsets[i] = buf->next - buf->code;
    printf("%p: emit code for opcode %d: ", buf->next, code[i]);
    switch (code[i]) {
      case MOVE_F64_C_S:
      case MOVE_U64_C_S:
        asmbuf_ins(buf, 2, 0x48B8); // mov rax, CONST
        asmbuf_immediate(buf, 8, code + i + 1);
        i += sizeof(uint64_t);
        asmbuf_ins(buf, 3, 0x488906); // mov [rsi], rax
        asmbuf_ins(buf, 4, 0x4883C608); // add rsi, 8
        break;
      case GEQ_U64_O_C:
        src = *((Address *)(code + i + 1));
        i += sizeof(Address);
        asmbuf_ins(buf, 2, 0x48B8); // mov rax, CONST
        asmbuf_immediate(buf, 8, code + i + 1);
        i += sizeof(uint64_t);
        asmbuf_ins(buf, 3, 0x4831D2); // xor rdx, rdx
        asmbuf_ins(buf, 3, 0x483987); // cmp [rdi+CONST], rax
        asmbuf_immediate(buf, 2, (char *)&src);
        asmbuf_ins(buf, 2, 0); // pad src to 4 bytes
        asmbuf_ins(buf, 2, 0x7203); // jnae 3
        asmbuf_ins(buf, 3, 0x48FFC2); // inc rdx
        break;
      case CONVU64_F64_O_S:
        src = *((Address *)(code + i + 1));
        i += sizeof(Address);
        asmbuf_ins(buf, 5, 0xF2480F2A87); // cvtsi2sd xmm0, qword [rdi+CONST]
        asmbuf_immediate(buf, 2, (char *)&src);
        asmbuf_ins(buf, 2, 0); // pad src to 4 bytes
        asmbuf_ins(buf, 4, 0xF20F1106); // movsd qword [rsi], xmm0
        asmbuf_ins(buf, 4, 0x4883C608); // add rsi, 8
        break;
      case ADD_F64_S_O:
        dest = *((Address *)(code + i + 1));
        i += sizeof(Address);
        asmbuf_ins(buf, 4, 0x4883EE08); // sub rsi, 8
        asmbuf_ins(buf, 4, 0xF20F1087); // movsd xmm0, qword [rdi+CONST]
        asmbuf_immediate(buf, 2, (char *)&dest);
        asmbuf_ins(buf, 2, 0); // pad dest to 4 bytes
        asmbuf_ins(buf, 4, 0xF20F5806); // addsd xmm0, [rsi]
        asmbuf_ins(buf, 4, 0xF20F1187); // movsd qword [rdi+CONST], xmm0
        asmbuf_immediate(buf, 2, (char *)&dest);
        asmbuf_ins(buf, 2, 0); // pad dest to 4 bytes
        break;
      case ADD_U64_C_O:
        dest = *((Address *)(code + i + 1));
        i += sizeof(Address);
        asmbuf_ins(buf, 2, 0x48B8); // mov rax, CONST
        asmbuf_immediate(buf, 8, code + i + 1);
        i += sizeof(uint64_t);
        asmbuf_ins(buf, 3, 0x480187); // add [rdi+CONST], rax
        asmbuf_immediate(buf, 2, (char *)&dest);
        asmbuf_ins(buf, 2, 0); // pad dest to 4 bytes
        break;
      case CJUMP:
        asmbuf_ins(buf, 3, 0x4885D2); // test rdx, rdx
        dest = *((Address *)(code + i + 1));
        if (dest <= i) {
          displacement = instruction_offsets[dest] - (buf->next - buf->code);
          printf("(displ:%d)", displacement);
        } else {
          offset_map_add(pending_offsets, dest, buf->next + 6);
          displacement = 0;
        }
        i += sizeof(Address);
        asmbuf_ins(buf, 2, 0x0F85); // jnz near CONST
        asmbuf_immediate(buf, 4, (char *)&displacement);
        break;
      case JUMP:
        dest = *((Address *)(code + i + 1));
        if (dest <= i) {
          displacement = instruction_offsets[dest] - (buf->next - buf->code) - 5;
          printf("(displ:%d)", displacement);
        } else {
          offset_map_add(pending_offsets, dest, buf->next + 5);
          displacement = 0;
        }
        i += sizeof(Address);
        asmbuf_ins(buf, 1, 0xE9); // jmp CONST
        asmbuf_immediate(buf, 4, (char *)&displacement);
        break;
      case EXIT:
        asmbuf_ins(buf, 4, 0x488B4708); // mov rax,[rdi + 8]
        asmbuf_ins(buf, 1, 0xC3); // ret
        break;
    }
    while (offset < buf->next) {
      printf("%02x", *(offset++));
    }
    printf("\n");
  }
  if (get_hash_map_size(pending_offsets.map)) {
    printf("error: invalid jump addresses\n");
    return 1;
  }
  delete_offset_map(pending_offsets);
  free(instruction_offsets);

  //asmbuf_ins(buf, 3, 0x488B07); // mov rax,[rdi]
  asmbuf_ins(buf, 4, 0x488B4708); // mov rax,[rdi + 8]
  asmbuf_ins(buf, 1, 0xC3); // ret
  printf("finalizing...\n");
  if (finalize_asmbuf(buf)) {
    printf("fail\n");
    return -1;
  }

  printf("dumping code:\n");
  uint8_t *c = buf->code;
  while (c < buf->next) {
    printf("%02x", *(c++));
  }
  printf("\n");

  printf("executing...\n");
  long (*obj)(void *) = (void *)buf->code;
  printf("%ld\n", obj(stack));

  delete_asmbuf(buf);

  
}
