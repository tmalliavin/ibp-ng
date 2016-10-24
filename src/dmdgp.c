
/* include the dmdgp header. */
#include "dmdgp.h"

/* dmdgp_write_header(): write a short header to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_header (FILE *fh, peptide_t *P,
                        const char *fname) {
  /* declare required variables:
   *  @i: residue index.
   */
  unsigned int i;

  /* write some introduction. */
  fprintf(fh, "# %s\n", fname);
  fprintf(fh, "# automatically generated by ibp-ng\n\n");

  /* begin a new section. */
  fprintf(fh, "# sequence:\n");
  fprintf(fh, "#");

  /* loop over the residues to write the sequence. */
  for (i = 0; i < P->n_res; i++) {
    /* print the current residue code. */
    fprintf(fh, " %s", resid_get_code3(P->res[i]));

    /* check if a line continuation is required. */
    if ((i + 1) % 15 == 0 && i < P->n_res - 1)
      fprintf(fh, "\n#");
  }

  /* begin a new section. */
  fprintf(fh, "\n\n");
  fprintf(fh, "# explicit sidechains:\n");
  fprintf(fh, "#");

  /* loop over the explicit sidechains. */
  for (i = 0; i < P->n_sc; i++) {
    /* print the residue identifier. */
    fprintf(fh, " %s%-4u", resid_get_code3(P->res[P->sc[i]]),
            P->sc[i] + 1);

    /* check if a line continuation is required. */
    if ((i + 1) % 5 == 0 && i < P->n_sc - 1)
      fprintf(fh, "\n#");
  }

  /* end the header. */
  fprintf(fh, "\n\n");

  /* return success. */
  return 1;
}

/* dmdgp_write_vertices(): write vertex information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_vertices (FILE *fh, peptide_t *P,
                          const char *fmt) {
  /* declare required variables:
   *  @vertex_fmt: vertex format string.
   *  @i: vertex index.
   */
  char vertex_fmt[256];
  unsigned int i;

  /* build the vertex format string. */
  snprintf(vertex_fmt, 256,
    "%s  *   *   *   # %%s%%-4u %%-4s (%%s)\n", fmt);

  /* begin the vertex section. */
  fprintf(fh, "# vertices: %u\n", P->n_atoms);
  fprintf(fh, "begin vertices\n");

  /* loop over the vertices of the graph. */
  for (i = 0; i < P->n_atoms; i++) {
    /* print the vertex entry. */
    fprintf(fh, vertex_fmt, i + 1,
            resid_get_code3(P->res[P->atoms[i].res_id]),
            P->atoms[i].res_id + 1,
            P->atoms[i].name,
            P->atoms[i].type);
  }

  /* end the vertex section. */
  fprintf(fh, "end vertices\n\n");

  /* return success. */
  return 1;
}

/* dmdgp_write_edges(): write edge information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_edges (FILE *fh, peptide_t *P, graph_t *G,
                       const char *fmt) {
  /* declare required variables:
   *  @ne, @ni: number of exact and interval edges.
   *  @efmt, @ifmt: edge format strings.
   *  @i, @j: vertex indices.
   */
  unsigned int i, j, ne, ni;
  char efmt[256], ifmt[256];

  /* get the graph edge counts. */
  graph_count_edges(G, &ne, &ni);

  /* begin the edge section. */
  fprintf(fh, "# exact edges:    %u\n", ne);
  fprintf(fh, "# interval edges: %u\n", ni);
  fprintf(fh, "begin edges\n");

  /* build the exact edge format string. */
  snprintf(efmt, 256,
    "%s%sD %%11.6lf             # %%s%%-4u %%-4s -- %%s%%-4u %%-4s\n",
    fmt, fmt);

  /* build the interval edge format string. */
  snprintf(ifmt, 256,
    "%s%sI %%11.6lf %%11.6lf # %%s%%-4u %%-4s -- %%s%%-4u %%-4s\n",
    fmt, fmt);

  /* loop over the edges of the graph. */
  for (i = 0; i < G->nv; i++) {
    for (j = i + 1; j < G->nv; j++) {
      /* get the edge type. */
      const value_type_t et = graph_has_edge(G, i, j);

      /* print the edge information. */
      if (et == VALUE_TYPE_SCALAR) {
        /* print the exact edge entry. */
        fprintf(fh, efmt, i + 1, j + 1, G->E[i + G->nv * j].l,
                resid_get_code3(P->res[P->atoms[i].res_id]),
                P->atoms[i].res_id + 1,
                P->atoms[i].name,
                resid_get_code3(P->res[P->atoms[j].res_id]),
                P->atoms[j].res_id + 1,
                P->atoms[j].name);
      }
      else if (et == VALUE_TYPE_INTERVAL) {
        /* print the interval edge entry. */
        fprintf(fh, ifmt, i + 1, j + 1,
                G->E[i + G->nv * j].l, G->E[i + G->nv * j].u,
                resid_get_code3(P->res[P->atoms[i].res_id]),
                P->atoms[i].res_id + 1,
                P->atoms[i].name,
                resid_get_code3(P->res[P->atoms[j].res_id]),
                P->atoms[j].res_id + 1,
                P->atoms[j].name);
      }
    }
  }

  /* end the edge section. */
  fprintf(fh, "end edges\n\n");

  /* return success. */
  return 1;
}

/* dmdgp_write_atoms(): write atom name information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_atoms (FILE *fh, peptide_t *P,
                       const char *fmt) {
  /* declare required variables:
   *  @hash: hash structure for organizing atom names.
   *  @i: atom index.
   */
  dmdgp_hash_t *hash;
  unsigned int i;

  /* allocate a new hash. */
  hash = dmdgp_hash_new();
  if (!hash)
    return 0;
  
  /* begin the atom name section. */
  fprintf(fh, "# atoms: %u\n", P->n_atoms);
  fprintf(fh, "begin atom_names\n");

  /* loop over the atoms of the peptide. */
  for (i = 0; i < P->n_atoms; i++) {
    /* add the atom to the hash. */
    if (!dmdgp_hash_add(hash, P->atoms[i].name, i + 1))
      throw("unable to add atom %u (%s) to hash",
            i + 1, P->atoms[i].name);
  }

  /* write the contents of the hash to the output file. */
  if (!dmdgp_hash_write(hash, fmt, fh))
    throw("unable to write atoms hash");

  /* end the atom name section. */
  fprintf(fh, "end atom_names\n\n");

  /* free the hash. */
  dmdgp_hash_free(hash);

  /* return success. */
  return 1;
}

/* dmdgp_write_residues(): write residue information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_residues (FILE *fh, peptide_t *P,
                          const char *fmt) {
  /* declare required variables:
   *  @hash: hash structure for organizing atom names.
   *  @resname: residue name string.
   *  @i: atom index.
   */
  const char *resname;
  dmdgp_hash_t *hash;
  unsigned int i;

  /* allocate a new hash. */
  hash = dmdgp_hash_new();
  if (!hash)
    return 0;

  /* begin the residue section. */
  fprintf(fh, "# residues: %u\n", P->n_res);
  fprintf(fh, "begin residues\n");

  /* loop over the atoms of the peptide. */
  for (i = 0; i < P->n_atoms; i++) {
    /* get the residue name string. */
    resname = resid_get_code3(P->res[P->atoms[i].res_id]);

    /* add the atom to the hash. */
    if (!dmdgp_hash_add(hash, resname, i + 1))
      throw("unable to add atom %u (%s) to hash", i + 1, resname);
  }

  /* write the contents of the hash to the output file. */
  if (!dmdgp_hash_write(hash, fmt, fh))
    throw("unable to write residues hash");

  /* end the residue section. */
  fprintf(fh, "end residues\n\n");

  /* free the hash. */
  dmdgp_hash_free(hash);

  /* return success. */
  return 1;
}

/* dmdgp_write_dihedrals(): write dihedral angle information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_dihedrals (FILE *fh, peptide_t *P,
                           const char *fmt) {
  /* declare required variables:
   *  @dihed_fmt: dihedral format string.
   *  @i: dihedral index.
   */
  char dihed_fmt[256];
  unsigned int i;

  /* begin the dihedral section. */
  fprintf(fh, "# dihedrals: %u\n", P->n_torsions);
  fprintf(fh, "# impropers: %u\n", P->n_impropers);
  fprintf(fh, "begin dihedral_angles\n");

  /* build the exact dihedral format string. */
  snprintf(dihed_fmt, 256, "%s%s%s%sD %%11.6lf\n",
           fmt, fmt, fmt, fmt);

  /* loop over the torsions of the peptide. */
  for (i = 0; i < P->n_torsions; i++) {
    /* skip intervals. */
    if (value_is_interval(P->torsions[i].ang)) continue;

    /* print the dihedral entry. */
    fprintf(fh, dihed_fmt,
            P->torsions[i].atom_id[0] + 1,
            P->torsions[i].atom_id[1] + 1,
            P->torsions[i].atom_id[2] + 1,
            P->torsions[i].atom_id[3] + 1,
            P->torsions[i].ang.l);
  }

  /* loop over the impropers of the peptide. */
  for (i = 0; i < P->n_impropers; i++) {
    /* skip intervals. */
    if (value_is_interval(P->impropers[i].ang)) continue;

    /* print the dihedral entry. */
    fprintf(fh, dihed_fmt,
            P->impropers[i].atom_id[0] + 1,
            P->impropers[i].atom_id[1] + 1,
            P->impropers[i].atom_id[2] + 1,
            P->impropers[i].atom_id[3] + 1,
            P->impropers[i].ang.l);
  }

  /* build the interval dihedral format string. */
  snprintf(dihed_fmt, 256, "%s%s%s%sI %%11.6lf %%11.6lf\n",
           fmt, fmt, fmt, fmt);

  /* loop over the torsions of the peptide. */
  for (i = 0; i < P->n_torsions; i++) {
    /* skip scalars. */
    if (value_is_scalar(P->torsions[i].ang)) continue;

    /* print the dihedral entry. */
    fprintf(fh, dihed_fmt,
            P->torsions[i].atom_id[0] + 1,
            P->torsions[i].atom_id[1] + 1,
            P->torsions[i].atom_id[2] + 1,
            P->torsions[i].atom_id[3] + 1,
            P->torsions[i].ang.l,
            P->torsions[i].ang.u);
  }

  /* loop over the impropers of the peptide. */
  for (i = 0; i < P->n_impropers; i++) {
    /* skip scalars. */
    if (value_is_scalar(P->impropers[i].ang)) continue;

    /* print the dihedral entry. */
    fprintf(fh, dihed_fmt,
            P->impropers[i].atom_id[0] + 1,
            P->impropers[i].atom_id[1] + 1,
            P->impropers[i].atom_id[2] + 1,
            P->impropers[i].atom_id[3] + 1,
            P->impropers[i].ang.l,
            P->impropers[i].ang.u);
  }

  /* end the dihedral section. */
  fprintf(fh, "end dihedral_angles\n\n");

  /* return success. */
  return 1;
}

/* dmdgp_write_order(): write graph order information to a DMDGP file.
 * see dmdgp_write() for more detailed information.
 */
int dmdgp_write_order (FILE *fh, peptide_t *P, graph_t *G,
                       const char *fmt) {
  /* declare required variables:
   *  @order_fmt: order format string.
   *  @iord: order atom index.
   *  @i: order index.
   */
  unsigned int i, iord;
  char order_fmt[256];

  /* build the order format string. */
  snprintf(order_fmt, 256, "%s # %%s%%-4u %%-4s\n", fmt);

  /* begin the order section. */
  fprintf(fh, "# reorder length: %u\n", G->n_order);
  fprintf(fh, "begin bp_order\n");

  /* loop over the ordering of the graph. */
  for (i = 0; i < G->n_order; i++) {
    /* get the order index. */
    iord = G->order[i];

    /* print the order entry. */
    fprintf(fh, order_fmt, iord + 1,
            resid_get_code3(P->res[P->atoms[iord].res_id]),
            P->atoms[iord].res_id + 1,
            P->atoms[iord].name);
  }

  /* end the order section. */
  fprintf(fh, "end bp_order\n\n");

  /* return success. */
  return 1;
}

/* dmdgp_write(): write an intermediate DMDGP file containing the general
 * graph structure of an iDMDGP instance.
 *
 * arguments:
 *  @fname: output filename to write data into.
 *  @P: pointer to the peptide structure to utilize.
 *  @G: pointer to the graph structure to utilize.
 *
 * returns:
 *  integer indicating whether (1) or not (0) the operation succeeded.
 */
int dmdgp_write (const char *fname, peptide_t *P, graph_t *G) {
  /* declare required variables:
   *  @n_fmt: number of characters required to represent atom indices.
   *  @i_fmt: general-purpose loop counter.
   *  @fmt: atom index string format.
   *  @fh: output file handle.
   */
  unsigned int i_fmt, n_fmt;
  char fmt[8];
  FILE *fh;

  /* determine the format count. */
  n_fmt = i_fmt = 1;
  while (i_fmt <= P->n_atoms) {
    /* increase the number of digits. */
    n_fmt++;
    i_fmt *= 10;
  }

  /* build the format string. */
  snprintf(fmt, 8, "%%-%uu", n_fmt);
  fmt[7] = '\0';

  /* open the output file. */
  fh = fopen(fname, "w");
  if (!fh)
    throw("unable to open '%s' for writing", fname);

  /* write the header. */
  if (!dmdgp_write_header(fh, P, fname))
    throw("unable to write header");

  /* write the vertex list. */
  if (!dmdgp_write_vertices(fh, P, fmt))
    throw("unable to write vertices");

  /* write the edge list. */
  if (!dmdgp_write_edges(fh, P, G, fmt))
    throw("unable to write edges");

  /* write the atom name list. */
  if (!dmdgp_write_atoms(fh, P, fmt))
    throw("unable to write atom names");

  /* write the residue list. */
  if (!dmdgp_write_residues(fh, P, fmt))
    throw("unable to write residues");

  /* write the dihedral list. */
  if (!dmdgp_write_dihedrals(fh, P, fmt))
    throw("unable to write dihedrals");

  /* write the graph ordering. */
  if (!dmdgp_write_order(fh, P, G, fmt))
    throw("unable to write order");

  /* close the output file. */
  fclose(fh);

  /* return success. */
  return 1;
}

