set(SQISIGN_KSV_SOURCE_ROOT
    "${PROJECT_SOURCE_DIR}/work/compact-d13"
    CACHE PATH "Reconstructed Compact-SQIsign v2 D13 source tree")
get_filename_component(SQISIGN_D13_ROOT
    "${SQISIGN_KSV_SOURCE_ROOT}" ABSOLUTE)
if(NOT EXISTS "${SQISIGN_D13_ROOT}/src/verification/ref/lvlx/verify.c")
    message(FATAL_ERROR "Reconstructed Compact-SQIsign v2 D13 tree is missing")
endif()

set(SQISIGN_D13_KSV_SOURCES
    src/common/generic/fips202.c
    src/common/generic/mem.c
    src/common/ref/aes_c.c
    src/common/ref/randombytes_ctrdrbg.c
    src/ec/ref/lvlx/basis.c
    src/ec/ref/lvlx/biextension.c
    src/ec/ref/lvlx/ec.c
    src/ec/ref/lvlx/ec_jac.c
    src/ec/ref/lvlx/isog_chains.c
    src/ec/ref/lvlx/xeval.c
    src/ec/ref/lvlx/xisog.c
    src/gf/ref/lvl1/fp_p5248_32.c
    src/gf/ref/lvlx/fp.c
    src/gf/ref/lvlx/fp2.c
    src/hd/ref/lvlx/hd.c
    src/hd/ref/lvlx/theta_isogenies.c
    src/hd/ref/lvlx/theta_structure.c
    src/id2iso/ref/lvlx/dim2id2iso.c
    src/id2iso/ref/lvlx/id2iso.c
    src/mp/ref/generic/mp.c
    src/precomp/ref/lvl1/e0_basis.c
    src/precomp/ref/lvl1/ec_params.c
    src/precomp/ref/lvl1/endomorphism_action.c
    src/precomp/ref/lvl1/hd_splitting_transforms.c
    src/precomp/ref/lvl1/quaternion_data.c
    src/precomp/ref/lvl1/torsion_constants.c
    src/quaternion/ref/generic/algebra.c
    src/quaternion/ref/generic/dim2.c
    src/quaternion/ref/generic/dim4.c
    src/quaternion/ref/generic/dpe.c
    src/quaternion/ref/generic/finit.c
    src/quaternion/ref/generic/hnf/hnf.c
    src/quaternion/ref/generic/hnf/hnf_internal.c
    src/quaternion/ref/generic/hnf/ibz_division.c
    src/quaternion/ref/generic/ideal.c
    src/quaternion/ref/generic/intbig.c
    src/quaternion/ref/generic/integers.c
    src/quaternion/ref/generic/lat_ball.c
    src/quaternion/ref/generic/lattice.c
    src/quaternion/ref/generic/lll/l2.c
    src/quaternion/ref/generic/lll/lll_applications.c
    src/quaternion/ref/generic/lll/ml2.c
    src/quaternion/ref/generic/lll/mlll.c
    src/quaternion/ref/generic/normeq.c
    src/signature/ref/lvlx/encode_signature.c
    src/signature/ref/lvlx/keygen.c
    src/signature/ref/lvlx/keygen_lowmem.c
    src/signature/ref/lvlx/sign.c
    src/sqisign_lowmem.c
    src/verification/ref/lvlx/common.c
    src/verification/ref/lvlx/encode_verification.c
    src/verification/ref/lvlx/verify.c
)

list(TRANSFORM SQISIGN_D13_KSV_SOURCES PREPEND "${SQISIGN_D13_ROOT}/")

add_library(sqisign_ksv_lvl1_ref32 STATIC ${SQISIGN_D13_KSV_SOURCES})

target_include_directories(sqisign_ksv_lvl1_ref32 PUBLIC
    ${SQISIGN_D13_ROOT}/include
    ${SQISIGN_D13_ROOT}/src/common/generic/include
    ${SQISIGN_D13_ROOT}/src/common/ref/include
    ${SQISIGN_D13_ROOT}/src/precomp/ref/lvl1/include
    ${SQISIGN_D13_ROOT}/src/mp/ref/generic/include
    ${SQISIGN_D13_ROOT}/src/gf/ref/include
    ${SQISIGN_D13_ROOT}/src/ec/ref/include
    ${SQISIGN_D13_ROOT}/src/hd/ref/include
    ${SQISIGN_D13_ROOT}/src/quaternion/ref/generic/include
    ${SQISIGN_D13_ROOT}/src/quaternion/ref/generic/internal_quaternion_headers
    ${SQISIGN_D13_ROOT}/src/id2iso/ref/include
    ${SQISIGN_D13_ROOT}/src/signature/ref/include
    ${SQISIGN_D13_ROOT}/src/verification/ref/include
)

target_compile_definitions(sqisign_ksv_lvl1_ref32 PUBLIC
    ENABLE_SIGN
    NDEBUG
    RADIX_32
    RANDOMBYTES_C
    SQISIGN_BUILD_TYPE_REF
    SQISIGN_GF_IMPL_REF
    SQISIGN_LOWMEM_ONLY
    SQISIGN_TARGET_OS_OTHER
    SQISIGN_VARIANT=lvl1
    TARGET_ARM
)

target_compile_options(sqisign_ksv_lvl1_ref32 PRIVATE
    -O3
    -fno-strict-aliasing
    -fstack-usage
    -ffunction-sections
    -fdata-sections
    -Wvla
    -Walloca
    -Wall
    -Wextra
    -Werror
    -Wno-unused-function
    -Wno-unused-parameter
    -Wno-maybe-uninitialized
)

set_target_properties(sqisign_ksv_lvl1_ref32 PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION FALSE
    POSITION_INDEPENDENT_CODE FALSE
)
