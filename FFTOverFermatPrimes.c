#include "flint/flint.h"
#include "flint/fmpz.h"
#include "flint/nmod_vec.h"
#include "flint/nmod_poly.h"
#include "flint/ulong_extras.h"
#include <stdbool.h>
#include <stdlib.h>

#define max(a, b) ((a > b) ? a : b)

mp_limb_t w2i(int power_of_two) {
  mp_limb_t answers[16] = {3, 9, 81, 6561, 54449, 61869, 19319, 15028, 282, 13987, 8224, 65529, 64, 4096, 65281, 65536};
  return answers[power_of_two];
}

mp_limb_t w_2i(int power_of_two) {
  mp_limb_t answers[16] = {21846,7282,8091,58355,3505,29606,23398,35843,64375,39504,64509,8192,64513,65521,256,65536};
  return answers[power_of_two];
}

void fft_(mp_ptr input, mp_ptr out, size_t elements, nmod_t modulus, int level, bool inverse, int stride) {
  if (elements > stride) {
    fft_(out, input, elements, modulus, level+1, inverse, stride*2);
    fft_(out+stride, input+stride, elements, modulus, level+1, inverse, stride*2);
    mp_limb_t w = ((inverse) ? w_2i(level) : w2i(level)), m = 1;
    for (int i = 0; i < elements; i += 2*stride) {
      mp_limb_t t = nmod_mul(m, out[i + stride], modulus);
      input[i / 2] = nmod_add(out[i], t, modulus);
      input[(elements + i) / 2] = nmod_sub(out[i], t, modulus);
      m = nmod_mul(m, w, modulus);
    }
  }
}

void fft(mp_ptr out, mp_ptr input, mp_limb_t elements, nmod_t modulus, int level, bool inverse) {
  _nmod_vec_set(out, input, elements);
  fft_(input, out, (size_t)elements, modulus, level, inverse, 1);
  _nmod_vec_set(out, input, elements);
  if (inverse) {
    /* printf("Polynomial product (pre-size-division): "); */
    /* for (int i = 0; i < elements; i++) { */
    /*   printf("%zu ", out[i]); */
    /* } */
    /* printf("\n"); */
    /* printf("Polynomial product (post-size-division): "); */
    for (int i = 0; i < elements; i++) {
      out[i] /= elements;
      /* printf("%zu ", out[i]); */
    }
    /* printf("\n"); */
  }
}

void fft_multiply(fmpz_t out, mp_ptr l, mp_size_t ln, mp_ptr r, mp_size_t rn) {
  nmod_t n; nmod_init(&n, 65537);
  mp_size_t ln_s = ln*(FLINT_BITS >> 1), rn_s = rn*(FLINT_BITS >> 1);
  ulong pl_s = 1 << (n_sizeinbase((ulong) ln_s+rn_s, 2)-1);
  int level = 16 - n_sizeinbase((mp_limb_t) pl_s, 2) + 1;
  /* printf("ln_s = %zu; rn_s = %zu; pl_s = %zu; level = %i\n", ln_s, rn_s, pl_s, level); */
  // Coefficients within one limb = FLINT_BITS/2;
  mp_ptr x = (mp_ptr) calloc(pl_s,sizeof(mp_limb_t));
  mp_ptr y = (mp_ptr) calloc(pl_s,sizeof(mp_limb_t));
  mp_ptr r1 = (mp_ptr) calloc(pl_s,sizeof(mp_limb_t));
  mp_ptr r2 = (mp_ptr) calloc(pl_s,sizeof(mp_limb_t));
  _nmod_poly_bit_unpack(x,ln_s, l, 2, n);
  _nmod_poly_bit_unpack(y,rn_s, r, 2, n);
  /* printf("x: "); */
  /* for (int i = 0; i < pl_s; i++) printf("%lu,", x[i]); */
  /* printf("\ny: "); */
  /* for (int i = 0; i < pl_s; i++) printf("%lu,", y[i]); */
  /* printf("\n"); */
  fft(r1, x, pl_s, n, level, false);
  fft(r2, y, pl_s, n, level, false);
  /* printf("Finished forward FFTs.\n"); */
  /* printf("r1: "); */
  /* for (int i = 0; i < pl_s; i++) { */
  /*   printf("%lu ", r1[i]); */
  /* } */
  /* printf("\nr2: "); */
  /* for (int i = 0; i < pl_s; i++) { */
  /*   printf("%lu ", r2[i]); */
  /* } */
  /* printf("\n"); */
  /* printf("Product: "); */
  for (int i = 0; i < pl_s; i++) {
    r1[i] = nmod_mul(r1[i], r2[i], n);
    /* printf("%zu ", r1[i]); */
  }
  /* printf("\nFinished pointwise multiplication.\n"); */
  fft(r2, r1, pl_s, n, level, true);
  /* printf("Finished backward FFT.\n"); */
  fmpz_zero(out);
  for (mp_limb_t i = pl_s; i > 0; i--) {
    mp_limb_t index = i - 1;
    fmpz_mul_2exp(out, out, 2);
    fmpz_add_ui(out, out, r2[index]);
  }
  /* printf("Finished interpolation.\n"); */
}

int main(int argc, char** argv) {
  if (argc != 3) { flint_printf("%s requires exactly 2 arguments, the multiplicands.\n", argv[0]); return 1; }
  fmpz_t x, y, o; fmpz_init(x); fmpz_init(y);
  fmpz_set_str(x, argv[1], 10);
  fmpz_set_str(y, argv[2], 10);
  mp_size_t xn = fmpz_size(x), yn = fmpz_size(y);
  fmpz_init2(o, xn+yn);
  mp_ptr l = (mp_ptr) calloc(xn, sizeof(mp_limb_t));
  mp_ptr r = (mp_ptr) calloc(yn, sizeof(mp_limb_t));
  fmpz_bit_pack(l, 0, xn*FLINT_BITS, x, 1, 0);
  fmpz_bit_pack(r, 0, yn*FLINT_BITS, y, 1, 0);
  fft_multiply(o, l, xn, r, yn);
  fmpz_print(o);
  flint_printf("\n");
  fmpz_clear(x); fmpz_clear(y); fmpz_clear(o); free(l); free(r);
  return 0;
}
