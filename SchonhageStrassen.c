#include <stdlib.h>
#include "flint/flint.h"
#include "flint/fft.h"
#include "flint/fmpz.h"
#include "gmp.h"

int main(int argc, char** argv) {
  mpz_t x, y;
  if (mpz_init_set_str(x, argv[1], 10) != 0) {
    flint_printf("expected decimal number as first argument, got %s", argv[1]);
    mpz_clear(x); return 1;
  }
  if (mpz_init_set_str(y, argv[2], 10) != 0) {
    flint_printf("expected decimal number as second argument, got %s", argv[2]);
    mpz_clear(x); mpz_clear(y); return 1;
  }
  const mp_limb_t* x_limbs = mpz_limbs_read(x);
  const mp_limb_t* y_limbs = mpz_limbs_read(y);
  size_t x_len = mpz_size(x);
  size_t y_len = mpz_size(y);
  mp_limb_t* result_limbs = (mp_limb_t*) calloc(x_len+y_len, sizeof(mp_limb_t));
  flint_mpn_mul_fft_main(result_limbs, x_limbs, x_len, y_limbs, y_len);
  fmpz_t r; fmpz_init(r);
  fmpz_bit_unpack_unsigned(r, result_limbs, 0, (x_len+y_len)*FLINT_BITS);
  fmpz_print(r); flint_printf("\n");
  mpz_clears(x, y, NULL);
  free(result_limbs);
  return 0;
}
