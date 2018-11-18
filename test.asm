push_sym foobar
push_u64 100
op_i32 arr
push_u64 15
push_f32 27.5
op_i32 cf4
op_val dup
op_i32 ast
push_u64 15
op_i32 agt
push_nil
push_i32 15
op_i32 con
push_i32 17
op_i32 con
op_i32 ctl
op_i32 chd
push_i32 6
push_i32 7
op_i32 mul
push_i32 2
push_i32 1
push_p f
call 12 0
push_i32 6
push_p factorial
call 4 0
exit

f:
lod4 0
lod4 4
op_i32 div
lod4 8
op_i32 sub
retp 4

factorial:
lod4 0
push_i32 2
op_i32 lth
jmpc factorial/done
lod4 0
lod4 0
push_i32 1
op_i32 sub
push_p factorial
call 4 0
op_i32 mul
retp 4
factorial/done:
push_i32 1
retp 4



