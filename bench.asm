push_u64 100000000 ; p  0: array_length
push_f64 0         ; p  8: sum
push_u64 0         ; p 16: iteration
push_u64 0         ; p 24: iterations
push_u64 0         ; p 32: innerloop
lod8 0
op_f64 arr ; r  1: array
; initialize array elements (index converted to double)
push_u64 0
initloop:
op_u64 dup
lod8 0
op_u64 geq
jmpc initdone
op_u64 dup
op_u64 dup
lodv 1
op_f64 cu8
op_f64 ast
push_u64 1
op_u64 add
jump initloop
initdone:
; read number of iterations from args[1]
lodv 0
push_u64 1
op_val agt
push_p atoi
call 0 1
op_u64 cu4
sto8 24
; calculate sum
outersumloop:
lod8 16
lod8 24
op_u64 geq
jmpc outersumdone
push_u64 0
sto8 32
innersumloop:
lod8 32
push_u64 1000000000
op_u64 geq
jmpc innersumdone
lod8 16
lod8 32
op_u64 add
lod8 0
op_u64 mod
lodv 1
op_f64 agt
lod8 8
op_f64 add
sto8 8
lod8 32
push_u64 1
op_u64 add
sto8 32
jump innersumloop
innersumdone:
lod8 16
push_u64 1
op_u64 add
sto8 16
jump outersumloop
outersumdone:
exit

atoi:
; converts string to u32
; ref parameter 0: array
; returns u32
push_u64 0 ; p 0: index
push_u32 0 ; p 8: result
atoi/start:
lodv 0
lod8 0
op_u8 agt
op_u8 dup
jmpc atoi/loop
lod4 8
retp 4
atoi/loop:
push_u8 48
op_u8 sub
op_u32 cu1
lod4 8
push_u32 10
op_u32 mul
op_u32 add
sto4 8
lod8 0
push_u64 1
op_u64 add
sto8 0
jump atoi/start
