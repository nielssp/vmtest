  push_f64 0          ; p  0: sum
; calculate sum
  push_u64 0          ; i
sumloop:
  op_u64 dup          ; duplicate i
  push_u64 1000000000
  op_u64 geq          ; i >= 1000000000
  jmpc sumdone
  op_u64 dup          ; duplicate i
  op_f64 cu8          ; convert to f64
  lod8 0              ; load sum
  op_f64 add
  sto8 0              ; store sum + (doble)i
  push_u64 1
  op_u64 add          ; add 1 to i
  jump sumloop
sumdone:
  exit
