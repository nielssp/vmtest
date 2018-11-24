int main() {
  double sum = 0.0;
  unsigned long iteration = 0;
  unsigned long iterations = 1000000000;
  while (iteration < iterations) {
    sum += iteration;
    iteration++;
  }
  return 0;
}
