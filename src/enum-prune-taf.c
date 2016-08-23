
/* include the enumerator header. */
#include "enum.h"
#include "enum-thread.h"
#include "enum-prune.h"

/* enum_prune_taf_t: structure for holding information required for
 * torsion angle feasibility pruning closures.
 */
typedef struct {
  /* @ntest, @nprune: test and prune counts of the current closure.
   * @n: array of backward step-counts for the prior atoms.
   * @bound: angle bound to check.
   */
  unsigned int ntest, nprune;
  peptide_dihed_t *arr;
  unsigned int n[4];
  value_t bound;
}
enum_prune_taf_t;

/* taf_level(): function utilized by taf_init() to check whether
 * all atoms in a torsion entry have been embedded.
 *
 * arguments:
 *  @order: graph order.
 *  @lev: current order level.
 *  @id: atom index to check.
 *
 * returns:
 *  graph order level (i.e. comparable to @lev) of the specified
 *  atom index.
 */
static inline unsigned int taf_level (unsigned int *order,
                                      unsigned int lev,
                                      unsigned int id) {
  /* declare required variables:
   *  @i: order loop counter.
   */
  unsigned int i;

  /* check for a match in the order prior to @lev. */
  for (i = 0; i <= lev; i++) {
    if (order[i] == id)
      return i;
  }

  /* return the index, which is now (@lev+1). */
  return i;
}

/* taf_init(): function utilized by:
 *  - enum_prune_dihe_init()
 *  - enum_prune_impr_init()
 */
static int taf_init (enum_t *E, peptide_dihed_t *arr, unsigned int n_arr,
                     unsigned int lev) {
  /* declare required variables:
   *  @i: peptide torsion/improper array index.
   *  @id: atom index that has just been embedded.
   *  @ids: atom indices in each torsion/improper.
   *  @data: closure data pointer.
   */
  unsigned int i, k, id, *ids, levs[4];
  enum_prune_taf_t *data;

  /* get the current atom index. */
  id = E->G->order[lev];

  /* loop over all torsions. */
  for (i = 0; i < n_arr; i++) {
    /* get the atom indices. */
    ids = arr[i].atom_id;

    /* skip if the last-embedded atom is not in the torsion. */
    if (id != ids[0] && id != ids[1] && id != ids[2] && id != ids[3])
      continue;

    /* get the graph level of each atom. */
    for (k = 0; k < 4; k++)
      levs[k] = taf_level(E->G->order, lev, ids[k]);

    /* skip if all other atoms in the order have not been embedded. */
    if (levs[0] > lev ||
        levs[1] > lev ||
        levs[2] > lev ||
        levs[3] > lev)
      continue;

    /* allocate the closure payload. */
    data = (enum_prune_taf_t*) malloc(sizeof(enum_prune_taf_t));
    if (!data)
      return 0;

    /* initialize the counters. */
    data->ntest = data->nprune = 0;
    data->arr = arr;

    /* store the offsets. */
    for (k = 0; k < 4; k++)
      data->n[k] = lev - levs[k];

    /* store the angle bound. */
    data->bound = value_scal(arr[i].ang, M_PI / 180.0);

    /* set the payload value and register the closure. */
    if (!enum_prune_add_closure(E, lev, enum_prune_taf, data))
      return 0;
  }

  /* return success. */
  return 1;
}

/* enum_prune_dihe_init(): initialize the dihedral feasibility pruner.
 */
int enum_prune_dihe_init (enum_t *E, unsigned int lev) {
  /* initialize using the array of torsional dihedrals. */
  return taf_init(E, E->P->torsions, E->P->n_torsions, lev);
}

/* enum_prune_impr_init(): initialize the improper feasibility pruner.
 */
int enum_prune_impr_init (enum_t *E, unsigned int lev) {
  /* initialize using the array of improper dihedrals. */
  return taf_init(E, E->P->impropers, E->P->n_impropers, lev);
}

/* enum_prune_taf(): determine whether an enumerator tree may be pruned
 * at a given node based on torsion angle feasibility.
 */
int enum_prune_taf (enum_t *E, enum_thread_t *th, void *data) {
  /* declare required variables:
   *  @taf_data: payload for dihe/impr closures.
   */
  enum_prune_taf_t *taf_data;
  vector_t x1, x2, x3, x4;
  vector_t b1, b2, b3;
  vector_t n1, n2, m;
  double x, y, omega;

  /* get the payload. */
  taf_data = (enum_prune_taf_t*) data;

  /* extract pretty handles to the atom positions. */
  x1 = th->state[th->level - taf_data->n[0]].pos;
  x2 = th->state[th->level - taf_data->n[1]].pos;
  x3 = th->state[th->level - taf_data->n[2]].pos;
  x4 = th->state[th->level - taf_data->n[3]].pos;

  /* compute the first vector in the three-vector system. */
  b1.x = x1.x - x2.x;
  b1.y = x1.y - x2.y;
  b1.z = x1.z - x2.z;

  /* compute the second vector in the three-vector system. */
  b2.x = x2.x - x3.x;
  b2.y = x2.y - x3.y;
  b2.z = x2.z - x3.z;

  /* compute the third vector in the three-vector system. */
  b3.x = x3.x - x4.x;
  b3.y = x3.y - x4.y;
  b3.z = x3.z - x4.z;

  /* compute the unit normal of the first plane. */
  vector_cross(&b1, &b2, &n1);
  vector_normalize(&n1);

  /* compute the unit normal of the second plane. */
  vector_cross(&b2, &b3, &n2);
  vector_normalize(&n2);

  /* compute the final vector of the orthonormal frame. */
  vector_normalize(&b2);
  vector_cross(&n1, &b2, &m);

  /* finally, compute the dihedral angle. */
  vector_dot(&n1, &n2, &x);
  vector_dot(&m, &n2, &y);
  omega = atan2(y, x);

  /* check if the computed dihedral angle is in bounds. */
  taf_data->ntest++;
  if (taf_data->bound.l - omega > E->ddf_tol ||
      omega - taf_data->bound.u > E->ddf_tol) {
    taf_data->nprune++;
    return 1;
  }

  /* do not prune. */
  return 0;
}

/* enum_prune_taf_report(): function utilized by:
 *  - enum_prune_dihe_init()
 *  - enum_prune_impr_init()
 */
void enum_prune_taf_report (enum_t *E, peptide_dihed_t *arr,
                            unsigned int lev, void *data) {
  /* get the closure payload. */
  enum_prune_taf_t *taf_data = (enum_prune_taf_t*) data;

  /* return if the requested array does not match, or if
   * no prunes were performed by the closure.
   */
  if (taf_data->arr != arr || !taf_data->nprune) return;

  /* get the atom indices. */
  unsigned int a0 = E->G->order[lev - taf_data->n[0]];
  unsigned int a1 = E->G->order[lev - taf_data->n[1]];
  unsigned int a2 = E->G->order[lev - taf_data->n[2]];
  unsigned int a3 = E->G->order[lev - taf_data->n[3]];

  /* get the residue indices. */
  unsigned int r0 = E->P->atoms[a0].res_id;
  unsigned int r1 = E->P->atoms[a1].res_id;
  unsigned int r2 = E->P->atoms[a2].res_id;
  unsigned int r3 = E->P->atoms[a3].res_id;

  /* get the atom names. */
  const char *atom0 = E->P->atoms[a0].name;
  const char *atom1 = E->P->atoms[a1].name;
  const char *atom2 = E->P->atoms[a2].name;
  const char *atom3 = E->P->atoms[a3].name;

  /* get the residue codes. */
  const char res0 = peptide_get_reschar(E->P, r0);
  const char res1 = peptide_get_reschar(E->P, r1);
  const char res2 = peptide_get_reschar(E->P, r2);
  const char res3 = peptide_get_reschar(E->P, r3);

  /* compute the percentage. */
  double f = ((double) taf_data->nprune) /
             ((double) taf_data->ntest) * 100.0;

  /* output the statistics. */
  printf("  %c%-4u %-4s | %c%-4u %-4s | "
         "%c%-4u %-4s | %c%-4u %-4s : "
         "%16u/%-16u  %3.0lf%%\n",
         res0, r0 + 1, atom0,
         res1, r1 + 1, atom1,
         res2, r2 + 1, atom2,
         res3, r3 + 1, atom3,
         taf_data->nprune, taf_data->ntest, f);
}

/* enum_prune_dihe_report(): output a report for the dihedral angle
 * feasbility (DIHE) pruning closure.
 */
void enum_prune_dihe_report (enum_t *E, unsigned int lev, void *data) {
  /* report using the array of proper dihedrals. */
  enum_prune_taf_report(E, E->P->torsions, lev, data);
}

/* enum_prune_impr_report(): output a report for the improper angle
 * feasbility (IMPR) pruning closure.
 */
void enum_prune_impr_report (enum_t *E, unsigned int lev, void *data) {
  /* report using the array of improper dihedrals. */
  enum_prune_taf_report(E, E->P->impropers, lev, data);
}

