  move_f64_c_s 0 ; p 0: sum
  move_u64_c_s 0 ; p 8: iteration
sumloop:
  geq_u64_o_c 8 1000000000 ; C = iteration >= 1000000000
  cjump sumdone            ; if (C) break
  convu64_f64_o_s 8        ; push((double)iteration)
  add_f64_s_o 0            ; sum += pop()
  add_u64_c_o 1 8          ; iteration += 1
  jump sumloop
sumdone:
  exit

