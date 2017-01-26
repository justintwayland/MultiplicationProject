#include "flint/fmpz.h"
#include "flint/fmpz_poly.h"

#define BITSPERCOEF FLINT_BITS

#define MAP_POLYNOMIAL_INPUTS(_)  _(x);	_(y)
#define MAP_POLYNOMIALS(f) f(x); f(y); f(r)

#define MAP_INTEGER_INPUTS(_) _(xi,1); _(yi,2)
#define MAP_INTEGERS(f) f(xi); f(yi); f(ri)

#define ZIP_INPUTS_WITH(f) f(x, xi); f(y, yi)

#define POLY_INIT(name) fmpz_poly_init(name)
#define INTEGER_INIT(name) fmpz_init(name)
#define SET_STRING(name, offset) fmpz_set_str(name, argv[offset], 10)
#define UNPACK(out, in) fmpz_poly_bit_unpack_unsigned(out, in, BITSPERCOEF);
#define CLEAR_INT(name) fmpz_free

int main(int argc, char** argv) {
  if (argc != 3) { flint_printf("Arguments: %s x y\n", argv[0]); return 1; } // Input checking
  fmpz_poly_t x, y, r; MAP_POLYNOMIALS(POLY_INIT); // Polynomial setup
  fmpz_t xi, yi, ri; MAP_INTEGERS(INTEGER_INIT); // Integer setup
  MAP_INTEGER_INPUTS(SET_STRING); // Integer initialization
  ZIP_INPUTS_WITH(UNPACK); // Split bigints into word-sized polynomials
  fmpz_poly_mul_karatsuba(r, x, y); // Perform Karatsuba Multiplication
  fmpz_one(ri); fmpz_mul_2exp(ri, ri, BITSPERCOEF);
  fmpz_poly_evaluate_horner_fmpz(ri, r, ri); // Extract the product by evaluating at the power of two originally used
  fmpz_print(ri); flint_printf("\n"); // Print out the resulting number
  MAP_POLYNOMIALS(fmpz_poly_clear); MAP_INTEGERS(fmpz_clear); // Cleanup
  return 0;
}
