push_u64 5
op_u64 dup
op_f64 arr
push_u64 0
loop:
op_u64 dup
lod8 0
op_u64 geq
jmpc done
op_u64 dup
op_u64 dup
op_val dup
op_f64 cu8
op_f64 ast
push_u64 1
op_u64 add
jump loop
done:
lodv 0
push_u64 1
op_val agt
push_p atoi
call 0 1
exit

atoi:
push_u64 0
push_u32 0
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
