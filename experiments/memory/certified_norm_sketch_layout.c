#include <stddef.h>
#include <stdio.h>

#include <id2iso.h>
#include <signature_lowmem.h>

#if defined(TARGET_ARM)
unsigned char certified_norm_sketch_probe_candidate
    [sizeof(find_uv_candidate_workspace_t)];
unsigned char certified_norm_sketch_probe_phase[sizeof(find_uv_phase_workspace_t)];
unsigned char certified_norm_sketch_probe_find_uv[sizeof(find_uv_workspace_t)];
unsigned char certified_norm_sketch_probe_operation
    [sizeof(protocols_operation_workspace_t)];
#endif

int
main(void)
{
    printf("ibz_t=%zu\n", sizeof(ibz_t));
    printf("small_vec=%zu\n", sizeof(find_uv_small_vec_t));
    printf("norm_sketch=%zu\n", sizeof(find_uv_norm_sketch_row_t));
    printf("norm_eval=%zu\n", sizeof(find_uv_norm_eval_workspace_t));
    printf("candidates=%zu align=%zu\n",
           sizeof(find_uv_candidate_workspace_t),
           _Alignof(find_uv_candidate_workspace_t));
    printf("lattice_state=%zu\n", sizeof(find_uv_lattice_state_t));
    printf("ml2_retry=%zu lattice_mul=%zu fixed_degree=%zu theta=%zu\n",
           sizeof(quat_ml2_retry_workspace_t),
           sizeof(quat_lattice_mul_workspace_t),
           sizeof(fixed_degree_isogeny_workspace_t),
           sizeof(theta_chain_workspace_t));
    printf("phase=%zu\n", sizeof(find_uv_phase_workspace_t));
    printf("find_uv=%zu align=%zu\n",
           sizeof(find_uv_workspace_t),
           _Alignof(find_uv_workspace_t));
    printf("operation=%zu align=%zu\n",
           sizeof(protocols_operation_workspace_t),
           _Alignof(protocols_operation_workspace_t));
    printf("candidate_offsets vecs=%zu sketch=%zu eval=%zu counts=%zu permutation=%zu\n",
           offsetof(find_uv_candidate_workspace_t, small_vecs),
           offsetof(find_uv_candidate_workspace_t, norm_sketch),
           offsetof(find_uv_candidate_workspace_t, eval),
           offsetof(find_uv_candidate_workspace_t, validated_counts),
           offsetof(find_uv_candidate_workspace_t, permutation));
    return 0;
}
