#include <fftw3.h>
#include <gmp.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "flint/ulong_extras.h"

#define CLEANUP \
  mpz_clear(n); \
  mpz_clear(c); \
  fftw_destroy_plan(p1); \
  fftw_destroy_plan(p2); \
  fftw_destroy_plan(inverse_p); \
  fftw_free(in); fftw_free(out); \
  fftw_free(other_in); mpz_clear(n1); mpz_clear(n2);
  

void complex_multiply(fftw_complex m, fftw_complex n, fftw_complex product) {
  //c.real = a.real*b.real - a.img*b.img;
  //c.img = a.img*b.real + a.real*b.img; 
  product[0] = m[0]*n[0] - m[1]*n[1];
  product[1] = m[1]*n[0] + m[0]*n[1];
}

int max(int a, int b) {
  return (a > b) ? a : b;
}

void numberToPolynomial(mpz_t n, unsigned int base, fftw_complex* out) {
  mpz_t r;
  mpz_init(r);
  int i = 0;
  while (mpz_sgn(n) != 0) {
    mpz_tdiv_qr_ui(n,r,n,base);
    out[i][0] = mpz_get_d(r);
    i++;
  }
  mpz_clear(r);
}

double round(double n) {
  double result = 0.0f;
  double fraction = modf(n, &result);
  if (fraction > 0.5) { result += 1.0; }
  return result;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    printf("Got %i arguments; expected 2", argc-1);
    return 1;
  }
  mpz_t n, n1, n2;
  int extra_bits = 0;
  mpz_init(n1); mpz_init(n2); mpz_init(n);
  mpz_set_str(n1, argv[1], 10);
  while (mpz_divisible_2exp_p(n1,1)) {
    mpz_tdiv_q_2exp(n1, n1, 1);
    extra_bits++;
  }
  mpz_set_str(n2, argv[2], 10);
  while (mpz_divisible_2exp_p(n2,1)) {
    mpz_tdiv_q_2exp(n2, n2, 1);
    extra_bits++;
  }
  int base = 256, bits = 8;
  int N = 1 << (n_sizeinbase(mpz_size(n1)*(mp_bits_per_limb/bits)+mpz_size(n2)*(mp_bits_per_limb/bits), 2)-1);
  // printf("n1_n = %zu, n2_n = %zu, N = %i\n", mpz_size(n1)*(mp_bits_per_limb/bits), mpz_size(n2)*(mp_bits_per_limb/bits), N);
  int i;
  fftw_complex *in, *out, *other_in;
  fftw_plan p1, p2;
  fftw_plan inverse_p;
  in = (fftw_complex*) calloc(N, sizeof(fftw_complex));
  out = (fftw_complex*) calloc(N, sizeof(fftw_complex));
  other_in = (fftw_complex*) calloc(N, sizeof(fftw_complex));
  numberToPolynomial(n1,base,in);
  numberToPolynomial(n2,base,other_in);
  /* printf("n1: "); */
  /* for (i=0; i < N; i++) { */
  /*   printf("%.0f ", in[i][0]); */
  /* } */
  /* printf("\nn2: "); */
  /* for (i=0; i < N; i++) { */
  /*   printf("%.0f ", other_in[i][0]); */
  /* } */
  /* printf("\n"); */
  //evaluate points at N roots of unity 
  p1 = fftw_plan_dft_1d(N, in, in, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p1); 
  p2 = fftw_plan_dft_1d(N, other_in, other_in, FFTW_FORWARD, FFTW_ESTIMATE);
  fftw_execute(p2);
  // pointwise multiply
  // printf("i in\t\tother_in\tproduct\n");
  for (i=0; i < N; i++) {
    complex_multiply(in[i], other_in[i], out[i]);
    // printf("%i %.0f+(%.0f)i\t%.0f+(%.0f)i\t%.0f+(%.0f)i\n", i, in_temp[i][0], in_temp[i][1], in_temp2[i][0], in_temp2[i][1], out[i][0], out[i][1]);
  }
  // printf("\n");
  //interpolate
  inverse_p = fftw_plan_dft_1d(N, out, out, FFTW_BACKWARD, FFTW_ESTIMATE); 
  fftw_execute(inverse_p); /* repeat as needed */

  // result needs to be divided by N
  // printf("Result: ");
  for (i=0; i < N; i++) {
    // printf("%.0f ", out[i][0]/N);
    out[i][0] = round(out[i][0]/(N));
  }
  // printf("\n");
  mpz_set_ui(n, 0); // n will be used to calculate the product
  mpz_t c; // c is the current_coefficient
  mpz_init(c);
  for (i=N; i > 0; i--) {
    int index = i - 1;
    mpz_set_d(c, out[index][0]);// Get the number to add
    mpz_mul_2exp(n, n, bits);
    mpz_add(n, n, c); //Add it to the product
  }
  mpz_mul_2exp(n, n, extra_bits); // Multiply extra bits back in
  char* result = mpz_get_str(NULL, 10, n);
  printf("%s\n",result);
  //cleanup
  CLEANUP; return 0;
}
