  move_f64_c_s 0 ; p 0: sum
  move_u64_c_s 0 ; p 8: iteration
sumloop:
  geq_u64_o_c_s 8 1000000000 ; push(iteration >= 1000000000)
  jump_s sumdone             ; if (pop()) break
  convu64_f64_o_s 8          ; push((double)iteration)
  add_f64_s_o_o 0 0          ; sum = pop() + sum
  add_u64_o_c_o 8 1 8        ; iteration = iteration + 1
  jump sumloop
sumdone:
  exit

