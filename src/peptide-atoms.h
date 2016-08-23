
/* ensure once-only inclusion. */
#ifndef __IBPNG_PEPTIDE_ATOMS_H__
#define __IBPNG_PEPTIDE_ATOMS_H__

/* function declarations (peptide-atoms.c): */

int peptide_atom_find (peptide_t *P,
                       unsigned int resid,
                       const char *name);

int peptide_atom_add (peptide_t *P,
                      unsigned int resid,
                      const char *name,
                      const char *type,
                      double mass,
                      double charge,
                      double radius);

int peptide_atom_modify (peptide_t *P,
                         unsigned int resid,
                         const char *name,
                         const char *type,
                         double mass,
                         double charge,
                         double radius);

int peptide_atom_delete (peptide_t *P,
                         unsigned int resid,
                         const char *name);

#endif /* !__IBPNG_PEPTIDE_ATOMS_H__ */

