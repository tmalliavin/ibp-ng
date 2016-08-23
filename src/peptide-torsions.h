
/* ensure once-only inclusion. */
#ifndef __IBPNG_PEPTIDE_TORSIONS_H__
#define __IBPNG_PEPTIDE_TORSIONS_H__

/* function declarations (peptide-torsions.c): */

int peptide_torsion_add (peptide_t *P,
                         unsigned int resid1, const char *name1,
                         unsigned int resid2, const char *name2,
                         unsigned int resid3, const char *name3,
                         unsigned int resid4, const char *name4);

int peptide_torsion_delete (peptide_t *P,
                            unsigned int resid1, const char *name1,
                            unsigned int resid2, const char *name2,
                            unsigned int resid3, const char *name3,
                            unsigned int resid4, const char *name4);

int peptide_torsion_delete_any (peptide_t *P,
                                unsigned int resid,
                                const char *name);

int peptide_graph_torsions (peptide_t *P, graph_t *G);

#endif /* !__IBPNG_PEPTIDE_TORSIONS_H__ */

