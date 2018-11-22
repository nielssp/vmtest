  push_f64 0          ; p  0: sum
; calculate sum
  push_u64 0
sumloop:
  op_u64 dup
  push_u64 1000000000 ; p 16: iterations
  op_u64 geq
  jmpc sumdone
  op_u64 dup
  op_f64 cu8
  lod8 0
  op_f64 add
  sto8 0
  push_u64 1
  op_u64 add
  jump sumloop
sumdone:
  exit
