// SPDX-License-Identifier: Apache-2.0
/* Compile-time/runtime parameter probe for the Level-I Cornacchia bound. */

#include "powmod_fixedwork_521.h"

#include <ec_params.h>
#include <encoded_sizes.h>
#include <hd.h>
#include <quaternion_data.h>

#include <stdio.h>

_Static_assert(TORSION_EVEN_POWER == 248,
               "Level-I even-torsion exponent changed");
_Static_assert(HD_extra_torsion == 2,
               "Level-I HD extra-torsion allowance changed");
_Static_assert(SQIsign_response_length == 126,
               "Level-I response-length bound changed");
_Static_assert(SQISIGN_SCA_FIXEDWORK_BITS == 521,
               "fixed-work arithmetic extent changed");

int
main(void)
{
    if (ibz_cmp(&QUAT_prime_cofactor, &ibz_const_zero) <= 0)
        return 1;
    printf("torsion_even_power=%d\n", TORSION_EVEN_POWER);
    printf("hd_extra_torsion=%d\n", HD_extra_torsion);
    printf("response_length=%d\n", SQIsign_response_length);
    printf("prime_cofactor_bits=%d\n", ibz_bitsize(&QUAT_prime_cofactor));
    printf("algebra_prime_bits=%d\n", ibz_bitsize(&QUATALG_PINFTY.p));
    printf("fixedwork_bits=%d\n", SQISIGN_SCA_FIXEDWORK_BITS);
    printf("ibz_bits=%d\n", IBZ_BITS);
    return 0;
}
