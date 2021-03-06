/* ------------------------------------------------------------
 This is the file "bem3d.c" of the H2Lib package.
 All rights reserved, Sven Christophersen 2011
 ------------------------------------------------------------ */

/* C STD LIBRARY */
#include <string.h>
/* CORE 0 */
#include "basic.h"
/* CORE 1 */
/* CORE 2 */
/* CORE 3 */
/* SIMPLE */
/* PARTICLES */
/* BEM */
#include "bem3d.h"

/* ------------------------------------------------------------
 Structs and typedefs
 ------------------------------------------------------------ */

/*
 * This constant defines the minimal width of an interval, that is used for
 * interpolation. If an interval @f$[a,b]@f$ is smaller than \ref INTERPOLATION_EPS_BEM3D
 * it is expanded to @f$[a - 0.5 \cdot \texttt{INTERPOLATION\_EPS\_BEM3D},
 * b + 0.5 \cdot \texttt{INTERPOLATION\_EPS\_BEM3D} ]@f$ .
 */
#define INTERPOLATION_EPS_BEM3D 1.0e-12

/*
 * Just an abbreviation for the struct _greencluster3d .
 */
typedef struct _greencluster3d greencluster3d;
/*
 * Pointer to a @ref greencluster3d object.
 */
typedef greencluster3d *pgreencluster3d;
/*
 * Pointer to a constant @ref greencluster3d object.
 */
typedef const greencluster3d *pcgreencluster3d;

/*
 * Just an abbreviation for the struct _greencluster3d .
 */
typedef struct _greenclusterbasis3d greenclusterbasis3d;
/*
 * Pointer to a @ref greenclusterbasis3d object.
 */
typedef greenclusterbasis3d *pgreenclusterbasis3d;
/*
 * Pointer to a constant @ref greenclusterbasis3d object.
 */
typedef const greenclusterbasis3d *pcgreenclusterbasis3d;

/*
 * @brief Substructure used for approximating @ref _hmatrix "h-", @ref
 * _uniformhmatrix "uniformh-" and @ref _h2matrix "h2matrices".
 *
 * This struct contains many needed parameters for the different approximation
 * techniques such as interpolation or green based methods.
 */
struct _aprxbem3d {

  /*
   * @brief Number of Tschebyscheff-interpolation points in each spatial dimension.
   */
  uint      m_inter;

  /*
   * @brief One dimensional Tschebyscheff-interpolation points in [-1,1].
   */
  preal     x_inter;

  /*
   * @brief Rank for interpolation based methods: @f$ k = m^2 @f$ .
   */
  uint      k_inter;

  /*
   * @brief Number of interval segments for green-quadrature.
   */
  uint      l_green;

  /*
   * @brief Number of gaussian quadrature points for green based methods.
   */
  uint      m_green;

  /*
   * @brief Product of @f$ m_{green} @f$ and @f$ l_{green} @f$.
   */
  uint      ml_green;

  /*
   * @brief Rank of green based methods.
   *
   * Depending on the utilized parameterization
   * the rank can vary. In case of cube parameterization @ref
   * build_bem3d_cube_quadpoints the rank applies to
   * @f$ k = 12 \, m^2 \cdot l^2 @f$ .
   */
  uint      k_green;

  /*
   * @brief @f$ \delta @f$ controls the distance between the bounding box of the current
   * cluster and the parameterization.
   *
   * This parameter is used as a relative
   * value. In fact the distance will be computed as @f$ \delta = \widetilde
   * \delta \cdot \operatorname{diam\_max\_cluser(t)} @f$ if cube parameterization
   * @ref build_bem3d_cube_quadpoints is used.
   */
  real      delta_green;

  /*
   * @brief One dimensional gaussian quadrature points in [-1,1] for green based
   * methods.
   */
  preal     t_green;

  /*
   * @brief One dimensional gaussian quadrature weight in [-1,1] for green based
   * methods.
   */

  preal     w_green;

  /*
   * @brief This is a callback function of type @ref quadpoints3d.
   *
   * It defines which
   * parameterization should be used for green based approximation techniques.
   */
  quadpoints3d quadpoints;

  /*
   * @brief Additional Information for greenhybrid methods.
   *
   * When using the greenhybrid methods, one needs to save the pivot elements
   * and the matrix @f$ V_t @f$ for each cluster. There we clone the structure of a
   * clustertree in a new struct _greencluster3d "greencluster3d" and add
   * these information to the clusters. <tt>grc_green</tt> is responsible for
   * the row clustertree.
   */
  pgreencluster3d grc_green;

  /*
   * @brief Additional Information for greenhybrid methods.
   *
   * When using the greenhybrid methods, one needs to save the pivot elements
   * and the matrix @f$ W_t @f$ for each cluster. There we clone the structure of a
   * clustertree in a new struct _greencluster3d "greencluster3d" and add
   * these information to the clusters. <tt>gcc_green</tt> is responsible for
   * the column clustertree.
   */
  pgreencluster3d gcc_green;

  /*
   * @brief Additional Information for greenhybrid methods.
   *
   * When using the greenhybrid methods, one needs to save the pivot elements
   * and the matrix @f$ V_t @f$ for each cluster. There we clone the structure of a
   * clusterbasis in a new struct _greenclusterbasis3d "greenclusterbasis3d" and add
   * these information to the clusters. <tt>grb_green</tt> is responsible for
   * the row clusterbasis. The matrix @f$ V_t @f$ is stored in the usual way inside
   * the clusterbasis itself.
   */
  pgreenclusterbasis3d grb_green;

  /*
   * @brief Additional Information for greenhybrid methods.
   *
   * When using the greenhybrid methods, one needs to save the pivot elements
   * and the matrix @f$ W_t @f$ for each cluster. There we clone the structure of a
   * clusterbasis in a new struct _greenclusterbasis3d "greenclusterbasis3d" and add
   * these information to the clusters. <tt>gcb_green</tt> is responsible for
   * the column clusterbasis. The matrix @f$ W_t @f$ is stored in the usual way inside
   * the clusterbasis itself.
   */
  pgreenclusterbasis3d gcb_green;

  /*
   * @brief Accuracy for ACA based algorithms
   */
  real      accur_aca;

  /*
   * @brief This flag indicated if blockwise recompression technique should be used or
   * not.
   *
   * Default value is <tt>recomp = false</tt>
   */
  bool      recomp;

  /*
   * @brief Accuracy for blockwise recompression.
   *
   * If <tt>recomp == true</tt> then <tt>accur_recomp</tt> controls the minimum
   * accuracy for each block within the @ref _hmatrix "hmatrix".
   */
  real      accur_recomp;

  /*
   * @brief This flag indicates whether \"coarsening\" should be used while constructing
   * @ref _hmatrix "hmatrices" or not.
   */
  bool      coarsen;

  /*
   * @brief Accuracy for coarsening.
   * If <tt>coarsen == true</tt> then <tt>accur_coarsen</tt> controls the minimum
   * accuracy for each coarsening step.
   */
  real      accur_coarsen;

  /*
   * @brief This flag indicated whether \"hierarchical compression\" should be used to
   * construct @ref _h2matrix "h2matrices"
   */
  bool      hiercomp;

  /*
   * @brief Accuracy for hierarchical recompression.
   *
   * If <tt>hiercomp == true</tt> then <tt>accur_hiercomp</tt> controls the minimum
   * accuracy for hierarchical compression technique.
   */
  real      accur_hiercomp;

  /*
   * @brief Additional information used by truncation-routines.
   */
  ptruncmode tm;
};

struct _parbem3d {
  /*
   * special members for different H- and H2-matrix approximations
   */

  phmatrix *hn;			/* temporary enumerated list of hmatrices */
  ph2matrix *h2n;		/* temporary enumerated list of h2matrices */
  pclusteroperator *rwn;	/* temporary enumerated list of clusteroperators for row-cluster */
  pclusteroperator *cwn;	/* temporary enumerated list of clusteroperators for col-cluster */
  uint     *leveln;		/* temporary list of levelnumber for each block in blocktree. */
  pgreencluster3d *grcn;
  uint      grcnn;
  pgreencluster3d *gccn;
  uint      gccnn;
  pgreenclusterbasis3d *grbn;
  uint      grbnn;
  pgreenclusterbasis3d *gcbn;
  uint      gcbnn;
};

struct _greencluster3d {
  uint     *xi;
	    /** local indices of pivot elements */
  uint     *xihat;
	       /** global indices of pivot elements */
  pamatrix  V;/** Interpolation operator */
  pccluster t; /** corresponding cluster */
  uint      sons;
	     /** number of sons for current cluster */
};

struct _greenclusterbasis3d {
  uint     *xi;
	    /** local indices of pivot elements */
  uint     *xihat;
	       /** global indices of pivot elements */
  pamatrix  Qinv;/** Triangular factor of QR-decomposition */
  pcclusterbasis cb; /** corresponding clusterbasis */
  uint      sons;
	     /** number of sons for current clusterbasis */
  uint      m;
};
static void
uninit_interpolation_bem3d(paprxbem3d aprx)
{
  if (aprx->x_inter != NULL) {
    freemem(aprx->x_inter);
    aprx->x_inter = NULL;
  }
  aprx->m_inter = 0;
  aprx->k_inter = 0;
}

static void
uninit_green_bem3d(paprxbem3d aprx)
{
  if (aprx->t_green != NULL) {
    freemem(aprx->t_green);
    aprx->t_green = NULL;
  }
  if (aprx->w_green != NULL) {
    freemem(aprx->w_green);
    aprx->w_green = NULL;
  }
  aprx->m_green = 0;
  aprx->l_green = 0;
  aprx->delta_green = 0.0;
  aprx->ml_green = 0;
  aprx->k_green = 0;

  aprx->grc_green = NULL;
  aprx->gcc_green = NULL;
  aprx->grb_green = NULL;
  aprx->gcb_green = NULL;

}

static void
uninit_aca_bem3d(paprxbem3d aprx)
{
  aprx->accur_aca = 0.0;
}

static void
uninit_recompression_bem3d(paprxbem3d aprx)
{
  aprx->recomp = false;
  aprx->accur_recomp = 0.0;
  aprx->coarsen = false;
  aprx->accur_coarsen = 0.0;
  aprx->hiercomp = false;
  aprx->accur_hiercomp = 0.0;
  if (aprx->tm != NULL) {
    del_truncmode(aprx->tm);
    aprx->tm = NULL;
  }
}

/* ------------------------------------------------------------
 Constructors and destructors
 ------------------------------------------------------------ */

static pgreencluster3d
new_greencluster3d(pccluster c)
{
  pgreencluster3d gc;
  uint      sons = c->sons;

  gc = (pgreencluster3d) allocmem(sizeof(greencluster3d));

  gc->V = new_amatrix(0, 0);
  gc->xi = NULL;
  gc->xihat = NULL;

  gc->t = c;
  gc->sons = sons;

  return gc;
}

static void
del_greencluster3d(pgreencluster3d gc)
{
  if (gc->xi != NULL) {
    freemem(gc->xi);
  }

  if (gc->xihat != NULL) {
    freemem(gc->xihat);
  }

  if (gc->V != NULL) {
    del_amatrix(gc->V);
  }

  freemem(gc);
}

static pgreenclusterbasis3d
new_greenclusterbasis3d(pcclusterbasis cb)
{
  pgreenclusterbasis3d gcb;
  uint      sons = cb->sons;

  gcb = (pgreenclusterbasis3d) allocmem(sizeof(greenclusterbasis3d));

  gcb->xi = NULL;
  gcb->xihat = NULL;
  gcb->Qinv = NULL;

  gcb->cb = cb;
  gcb->sons = sons;
  gcb->m = 0;

  return gcb;
}

static void
del_greenclusterbasis3d(pgreenclusterbasis3d gcb)
{
  if (gcb->xi != NULL) {
    freemem(gcb->xi);
  }
  if (gcb->xihat != NULL) {
    freemem(gcb->xihat);
  }

  if (gcb->Qinv != NULL) {
    del_amatrix(gcb->Qinv);
    gcb->Qinv = NULL;
  }

  freemem(gcb);
}

static plistnode
new_listnode(uint data, plistnode next)
{
  plistnode ln;

  ln = (plistnode) allocmem(sizeof(listnode));
  ln->data = data;
  ln->next = next;

  return ln;
}

static void
del_listnode(plistnode ln)
{
  plistnode ln1;

  if (ln) {
    ln1 = ln;
    while (ln1->next) {
      ln = ln1;
      ln1 = ln1->next;
      freemem(ln);
    }
    freemem(ln1);
  }

}

static paprxbem3d
new_aprxbem3d()
{
  paprxbem3d aprx;

  aprx = (paprxbem3d) allocmem(sizeof(aprxbem3d));

  /* Interpolation */
  aprx->x_inter = NULL;
  aprx->m_inter = 0;
  aprx->k_inter = 0;

  /* Green */
  aprx->m_green = 0;
  aprx->l_green = 0;
  aprx->ml_green = 0;
  aprx->k_green = 0;
  aprx->delta_green = 0.0;
  aprx->t_green = NULL;
  aprx->w_green = NULL;
  aprx->quadpoints = NULL;
  aprx->grc_green = NULL;
  aprx->gcc_green = NULL;
  aprx->grb_green = NULL;
  aprx->gcb_green = NULL;

  /* ACA */
  aprx->accur_aca = 0.0;

  /* Recompression */
  aprx->recomp = false;
  aprx->accur_recomp = 0.0;
  aprx->coarsen = false;
  aprx->accur_coarsen = 0.0;
  aprx->hiercomp = false;
  aprx->accur_hiercomp = 0.0;
  aprx->tm = NULL;

  return aprx;
}

static void
del_aprxbem3d(paprxbem3d aprx)
{
  uninit_interpolation_bem3d(aprx);
  uninit_green_bem3d(aprx);
  uninit_aca_bem3d(aprx);
  uninit_recompression_bem3d(aprx);

  freemem(aprx);
}

static pkernelbem3d
new_kernelbem3d()
{
  pkernelbem3d kernels;

  kernels = allocmem(sizeof(kernelbem3d));

  kernels->kernel_row = NULL;
  kernels->kernel_col = NULL;
  kernels->dnz_kernel_row = NULL;
  kernels->dnz_kernel_col = NULL;
  kernels->fundamental_row = NULL;
  kernels->fundamental_col = NULL;
  kernels->dnz_fundamental_row = NULL;
  kernels->dnz_fundamental_col = NULL;
  kernels->lagrange_row = NULL;
  kernels->lagrange_col = NULL;

  return kernels;
}

static void
del_kernelbem3d(pkernelbem3d kernels)
{
  freemem(kernels);
}

static pparbem3d
new_parbem3d()
{
  pparbem3d par;

  par = (pparbem3d) allocmem(sizeof(parbem3d));

  par->hn = NULL;
  par->h2n = NULL;
  par->rwn = NULL;
  par->cwn = NULL;
  par->leveln = NULL;
  par->grcn = NULL;
  par->grcnn = 0;
  par->gccn = NULL;
  par->gccnn = 0;
  par->grbn = NULL;
  par->grbnn = 0;
  par->gcbn = NULL;
  par->gcbnn = 0;

  return par;
}

static void
del_parbem3d(pparbem3d par)
{
  uint      n, i;

  if (par->hn != NULL) {
    freemem(par->hn);
  }

  if (par->h2n != NULL) {
    freemem(par->h2n);
  }

  if (par->rwn != NULL) {
    freemem(par->rwn);
  }

  if (par->cwn != NULL) {
    freemem(par->cwn);
  }

  if (par->leveln != NULL) {
    freemem(par->leveln);
  }

  /*
   * greencluster
   */

  n = par->grcnn;
  if (par->grcn != NULL && n != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grcn[i] != NULL) {
	del_greencluster3d(par->grcn[i]);
      }
    }
    freemem(par->grcn);
  }

  n = par->gccnn;
  if (par->gccn != NULL && n != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gccn[i] != NULL) {
	del_greencluster3d(par->gccn[i]);
      }
    }
    freemem(par->gccn);
  }

  /*
   * greenclusterbasis
   */

  n = par->grbnn;
  if (par->grbn != NULL && n != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grbn[i] != NULL) {
	del_greenclusterbasis3d(par->grbn[i]);
      }
    }
    freemem(par->grbn);
  }

  n = par->gcbnn;
  if (par->gcbn != NULL && n != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gcbn[i] != NULL) {
	del_greenclusterbasis3d(par->gcbn[i]);
      }
    }
    freemem(par->gcbn);
  }

  freemem(par);
}

pbem3d
new_bem3d(pcsurface3d gr)
{
  pbem3d    bem;

  bem = (pbem3d) allocmem(sizeof(bem3d));

  bem->gr = gr;

  bem->mass = NULL;
  bem->v2t = NULL;
  bem->alpha = 0.0;

  bem->N_neumann = 0;
  bem->basis_neumann = BASIS_NONE_BEM3D;
  bem->N_dirichlet = 0;
  bem->basis_dirichlet = BASIS_NONE_BEM3D;

  bem->nearfield = NULL;
  bem->farfield_rk = NULL;
  bem->farfield_u = NULL;
  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;

  bem->aprx = new_aprxbem3d();
  bem->kernels = new_kernelbem3d();
  bem->par = new_parbem3d();

  return bem;
}

void
del_bem3d(pbem3d bem)
{
  uint      i, n;

  if (bem->mass != NULL) {
    freemem(bem->mass);
  }

  if (bem->sq != NULL) {
    del_singquad2d(bem->sq);
  }

  del_aprxbem3d(bem->aprx);
  del_kernelbem3d(bem->kernels);
  del_parbem3d(bem->par);

  n = bem->gr->vertices;
  if (bem->v2t != NULL) {
    for (i = 0; i < n; ++i) {
      if (bem->v2t[i] != NULL) {
	del_listnode(bem->v2t[i]);
      }
    }
    freemem(bem->v2t);
  }

  freemem(bem);
}

pvert_list
new_vert_list(pvert_list next)
{
  pvert_list vl;

  vl = (pvert_list) allocmem(sizeof(vert_list));
  vl->v = 0;
  vl->next = next;

  return vl;
}

void
del_vert_list(pvert_list vl)
{
  pvert_list vl1;

  assert(vl != NULL);

  vl1 = vl;
  while (vl1) {
    vl = vl1->next;
    freemem(vl1);
    vl1 = vl;
  }
}

ptri_list
new_tri_list(ptri_list next)
{
  ptri_list tl;

  tl = (ptri_list) allocmem(sizeof(tri_list));
  tl->t = 0;
  tl->vl = NULL;
  tl->next = next;

  return tl;
}

void
del_tri_list(ptri_list tl)
{
  ptri_list tl1;

  assert(tl != NULL);

  tl1 = tl;
  while (tl1) {
    tl = tl1->next;
    del_vert_list(tl1->vl);
    freemem(tl1);
    tl1 = tl;
  }
}

void
setup_vertex_to_triangle_map_bem3d(pbem3d bem)
{
  pcsurface3d gr = bem->gr;
  const uint vertices = gr->vertices;
  const uint triangles = gr->triangles;
  uint(*tri)[3] = gr->t;

  plistnode *v2t = allocmem(vertices * sizeof(plistnode));
  uint      i, *t;

  /* TODO no sentinel node */
  for (i = 0; i < vertices; ++i) {
    v2t[i] = new_listnode(0, NULL);
  }

  for (i = 0; i < triangles; ++i) {
    t = tri[i];

    v2t[t[0]] = new_listnode(i, v2t[t[0]]);
    v2t[t[1]] = new_listnode(i, v2t[t[1]]);
    v2t[t[2]] = new_listnode(i, v2t[t[2]]);
  }

  bem->v2t = v2t;
}

/* ------------------------------------------------------------
 Methods to build clustertrees
 ------------------------------------------------------------ */

pclustergeometry
build_bem3d_const_clustergeometry(pcbem3d bem, uint ** idx)
{
  pcsurface3d gr = bem->gr;
  const     real(*x)[3] = (const real(*)[3]) gr->x;
  const     uint(*t)[3] = (const uint(*)[3]) gr->t;
  const real *g = (const real *) gr->g;
  uint      triangles = gr->triangles;

  pclustergeometry cg;
  uint      i;

  cg = new_clustergeometry(3, triangles);
  *idx = allocuint(triangles);

  for (i = 0; i < triangles; i++) {
    (*idx)[i] = i;

    /* Center of gravity as characteristic point */
    cg->x[i][0] = (x[t[i][0]][0] + x[t[i][1]][0] + x[t[i][2]][0]) / 3.0;
    cg->x[i][1] = (x[t[i][0]][1] + x[t[i][1]][1] + x[t[i][2]][1]) / 3.0;
    cg->x[i][2] = (x[t[i][0]][2] + x[t[i][1]][2] + x[t[i][2]][2]) / 3.0;

    /* Lower front left corner of bounding box */
    cg->smin[i][0] = REAL_MIN3(x[t[i][0]][0], x[t[i][1]][0], x[t[i][2]][0]);
    cg->smin[i][1] = REAL_MIN3(x[t[i][0]][1], x[t[i][1]][1], x[t[i][2]][1]);
    cg->smin[i][2] = REAL_MIN3(x[t[i][0]][2], x[t[i][1]][2], x[t[i][2]][2]);

    /* Upper back right corner of bounding box */
    cg->smax[i][0] = REAL_MAX3(x[t[i][0]][0], x[t[i][1]][0], x[t[i][2]][0]);
    cg->smax[i][1] = REAL_MAX3(x[t[i][0]][1], x[t[i][1]][1], x[t[i][2]][1]);
    cg->smax[i][2] = REAL_MAX3(x[t[i][0]][2], x[t[i][1]][2], x[t[i][2]][2]);

    cg->w[i] = g[i];
  }

  return cg;
}

pclustergeometry
build_bem3d_linear_clustergeometry(pcbem3d bem, uint ** idx)
{
  pcsurface3d gr = bem->gr;
  const     real(*x)[3] = (const real(*)[3]) gr->x;
  const     uint(*t)[3] = (const uint(*)[3]) gr->t;
  uint      triangles = gr->triangles;
  uint      vertices = gr->vertices;

  pclustergeometry cg;
  uint      tv0, tv1, tv2, i, j;
  real      tmin[3], tmax[3];

  cg = new_clustergeometry(3, vertices);
  *idx = allocuint(vertices);

  for (i = 0; i < vertices; ++i) {
    (*idx)[i] = i;

    /* Vertices as characteristic points */
    cg->x[i][0] = x[i][0];
    cg->x[i][1] = x[i][1];
    cg->x[i][2] = x[i][2];
    cg->smin[i][0] = x[i][0];
    cg->smin[i][1] = x[i][1];
    cg->smin[i][2] = x[i][2];
    cg->smax[i][0] = x[i][0];
    cg->smax[i][1] = x[i][1];
    cg->smax[i][2] = x[i][2];
  }

  for (i = 0; i < triangles; i++) {
    tv0 = t[i][0];
    tv1 = t[i][1];
    tv2 = t[i][2];

    /* Lower front left corner of bounding box for current triangle */
    tmin[0] = REAL_MIN3(x[tv0][0], x[tv1][0], x[tv2][0]);
    tmin[1] = REAL_MIN3(x[tv0][1], x[tv1][1], x[tv2][1]);
    tmin[2] = REAL_MIN3(x[tv0][2], x[tv1][2], x[tv2][2]);

    /* Upper back right corner of bounding box for current triangle */
    tmax[0] = REAL_MAX3(x[tv0][0], x[tv1][0], x[tv2][0]);
    tmax[1] = REAL_MAX3(x[tv0][1], x[tv1][1], x[tv2][1]);
    tmax[2] = REAL_MAX3(x[tv0][2], x[tv1][2], x[tv2][2]);

    /* update the bounding box for every vertex of the current triangle */
    for (j = 0; j < 3; ++j) {
      tv0 = t[i][j];

      /* Lower front left corner of bounding box */
      cg->smin[tv0][0] = REAL_MIN(cg->smin[tv0][0], tmin[0]);
      cg->smin[tv0][1] = REAL_MIN(cg->smin[tv0][1], tmin[1]);
      cg->smin[tv0][2] = REAL_MIN(cg->smin[tv0][2], tmin[2]);

      /* Upper back right corner of bounding box */
      cg->smax[tv0][0] = REAL_MAX(cg->smax[tv0][0], tmax[0]);
      cg->smax[tv0][1] = REAL_MAX(cg->smax[tv0][1], tmax[1]);
      cg->smax[tv0][2] = REAL_MAX(cg->smax[tv0][2], tmax[2]);
    }
  }

  return cg;
}

pclustergeometry
build_bem3d_clustergeometry(pcbem3d bem, uint ** idx,
			    basisfunctionbem3d basis)
{
  pclustergeometry cg;

  if (basis == BASIS_CONSTANT_BEM3D) {
    cg = build_bem3d_const_clustergeometry(bem, idx);
  }
  else {
    assert(basis == BASIS_LINEAR_BEM3D);
    cg = build_bem3d_linear_clustergeometry(bem, idx);
  }

  return cg;
}

pcluster
build_bem3d_cluster(pcbem3d bem, uint clf, basisfunctionbem3d basis)
{
  pclustergeometry cg;
  pcluster  c;
  uint     *idx;
  uint      n;

  if (basis == BASIS_CONSTANT_BEM3D) {
    cg = build_bem3d_const_clustergeometry(bem, &idx);
    n = bem->gr->triangles;
  }
  else {
    assert(basis == BASIS_LINEAR_BEM3D);
    cg = build_bem3d_linear_clustergeometry(bem, &idx);
    n = bem->gr->vertices;
  }

  c = build_adaptive_cluster(cg, n, idx, clf);

  del_clustergeometry(cg);

  return c;
}

static void
setup_interpolation_bem3d(paprxbem3d aprx, uint m)
{
  real      e;
  uint      i;

  uninit_interpolation_bem3d(aprx);

  aprx->x_inter = allocreal(m);
  aprx->m_inter = m;
  aprx->k_inter = m * m * m;

  /* build tschebyscheff-points */
  e = 1.0 / (2.0 * m);

  for (i = 0; i < m; ++i) {
    aprx->x_inter[i] = cos(M_PI * (2.0 * i * e + e));
  }

}

static void
setup_green_bem3d(paprxbem3d aprx, uint m, uint l, real delta,
		  quadpoints3d quadpoints)
{
  uint      i, j;
  real      h, c;
  real     *s, *ht, *hw;

  uninit_green_bem3d(aprx);

  s = allocreal(l + 1);
  ht = allocreal(m);
  hw = allocreal(m);

  aprx->m_green = m;
  aprx->l_green = l;
  aprx->delta_green = delta;
  aprx->ml_green = m * l;
  if (quadpoints == build_bem3d_cube_quadpoints) {
    aprx->k_green = 12 * m * l * m * l;
  }

  aprx->quadpoints = quadpoints;

  /* quadrature points and weights */
  if (m == 1) {
    ht[0] = 0.0;
    hw[0] = 2.0;
  }
  else {
    assemble_gauss(m, ht, hw);
  }

  /* partitioning the intervall */
  c = 2.0 / (real) l;
  for (i = 0; i <= l; i++) {
    s[i] = -1.0 + (c * i);
  }

  aprx->t_green = allocreal(aprx->ml_green);
  aprx->w_green = allocreal(aprx->ml_green);

  /* adjust quadrature points and weights */
  for (j = 1; j <= l; j++) {
    h = 0.5 * (s[j] - s[j - 1]);
    for (i = 0; i < m; i++) {
      aprx->t_green[i + (j - 1) * m] = h * ht[i] + 0.5 * (s[j] + s[j - 1]);
      aprx->w_green[i + (j - 1) * m] = h * hw[i];
    }
  }

  freemem(s);
  freemem(ht);
  freemem(hw);
}

static void
setup_aca_bem3d(paprxbem3d aprx, real accur)
{
  assert(accur > 0.0);

  uninit_aca_bem3d(aprx);

  aprx->accur_aca = accur;
}

void
build_bem3d_cube_quadpoints(pcbem3d bem, const real a[3], const real b[3],
			    const real delta, real(**Z)[3], real(**N)[3])
{
  pcaprxbem3d aprx = bem->aprx;
  const real *t = aprx->t_green;
  const real *w = aprx->w_green;
  const uint ml = aprx->ml_green;
  const uint k2 = aprx->k_green * 0.5;

  uint      gammanumber, mu1, mu2, nu;
  real      velo, factor, u, v;

  *N = (real(*)[3]) allocreal(3 * k2);
  *Z = (real(*)[3]) allocreal(3 * k2);

  nu = 0;

  for (gammanumber = 1; gammanumber <= 6; gammanumber++) {

    switch (gammanumber) {
    case 1:
      velo = (b[0] - a[0] + 2.0 * delta) * (b[1] - a[1] + 2.0 * delta) * 0.25;
      break;
    case 2:
      velo = (b[1] - a[1] + 2.0 * delta) * (b[2] - a[2] + 2.0 * delta) * 0.25;
      break;
    case 3:
      velo = (b[0] - a[0] + 2.0 * delta) * (b[1] - a[1] + 2.0 * delta) * 0.25;
      break;
    case 4:
      velo = (b[1] - a[1] + 2.0 * delta) * (b[2] - a[2] + 2.0 * delta) * 0.25;
      break;
    case 5:
      velo = (b[0] - a[0] + 2.0 * delta) * (b[2] - a[2] + 2.0 * delta) * 0.25;
      break;
    case 6:
      velo = (b[0] - a[0] + 2.0 * delta) * (b[2] - a[2] + 2.0 * delta) * 0.25;
      break;
    default:
      printf("ERROR: unknown gammanumber!\n");
      exit(0);
      break;
    }

    for (mu1 = 0; mu1 < ml; mu1++) {
      u = 0.5 * t[mu1];
      for (mu2 = 0; mu2 < ml; mu2++) {
	v = 0.5 * t[mu2];
	factor = velo * w[mu1] * w[mu2];

	(*N)[nu][0] = 0.0;
	(*N)[nu][1] = 0.0;
	(*N)[nu][2] = 0.0;

	switch (gammanumber) {
	case 1:
	  (*Z)[nu][0] =
	    (0.5 * (b[0] + a[0])) + (b[0] - a[0] + 2.0 * delta) * u;
	  (*Z)[nu][1] =
	    (0.5 * (b[1] + a[1])) + (b[1] - a[1] + 2.0 * delta) * v;
	  (*Z)[nu][2] = a[2] - delta;
	  (*N)[nu][2] = -factor;
	  break;
	case 2:
	  (*Z)[nu][0] = b[0] + delta;
	  (*Z)[nu][1] =
	    (0.5 * (b[1] + a[1])) + (b[1] - a[1] + 2.0 * delta) * u;
	  (*Z)[nu][2] =
	    (0.5 * (b[2] + a[2])) + (b[2] - a[2] + 2.0 * delta) * v;
	  (*N)[nu][0] = factor;
	  break;
	case 3:
	  (*Z)[nu][0] =
	    (0.5 * (b[0] + a[0])) + (b[0] - a[0] + 2.0 * delta) * u;
	  (*Z)[nu][1] =
	    (0.5 * (b[1] + a[1])) + (b[1] - a[1] + 2.0 * delta) * v;
	  (*Z)[nu][2] = b[2] + delta;
	  (*N)[nu][2] = factor;
	  break;
	case 4:
	  (*Z)[nu][0] = a[0] - delta;
	  (*Z)[nu][1] =
	    (0.5 * (b[1] + a[1])) + (b[1] - a[1] + 2.0 * delta) * u;
	  (*Z)[nu][2] =
	    (0.5 * (b[2] + a[2])) + (b[2] - a[2] + 2.0 * delta) * v;
	  (*N)[nu][0] = -factor;
	  break;
	case 5:
	  (*Z)[nu][0] =
	    (0.5 * (b[0] + a[0])) + (b[0] - a[0] + 2.0 * delta) * u;
	  (*Z)[nu][1] = a[1] - delta;
	  (*Z)[nu][2] =
	    (0.5 * (b[2] + a[2])) + (b[2] - a[2] + 2.0 * delta) * v;
	  (*N)[nu][1] = -factor;
	  break;
	case 6:
	  (*Z)[nu][0] =
	    (0.5 * (b[0] + a[0])) + (b[0] - a[0] + 2.0 * delta) * u;
	  (*Z)[nu][1] = b[1] + delta;
	  (*Z)[nu][2] =
	    (0.5 * (b[2] + a[2])) + (b[2] - a[2] + 2.0 * delta) * v;
	  (*N)[nu][1] = factor;
	  break;
	}

	nu++;
      }
    }
  }

  assert(nu == k2);
}

static void
assemble_interpoints3d_array(pcbem3d bem, pccluster t, real(*X)[3])
{
  pcaprxbem3d aprx = bem->aprx;
  real     *x = aprx->x_inter;
  uint      m = aprx->m_inter;
  uint      k = aprx->k_inter;
  real      ax = t->bmin[0];
  real      bx = t->bmax[0];
  real      ay = t->bmin[1];
  real      by = t->bmax[1];
  real      az = t->bmin[2];
  real      bz = t->bmax[2];

  real      cx, dx, cy, dy, cz, dz;
  uint      i, j, l, index;

  if (bx - ax < INTERPOLATION_EPS_BEM3D) {
    bx += INTERPOLATION_EPS_BEM3D;
    ax -= INTERPOLATION_EPS_BEM3D;
  }
  if (by - ay < INTERPOLATION_EPS_BEM3D) {
    by += INTERPOLATION_EPS_BEM3D;
    ay -= INTERPOLATION_EPS_BEM3D;
  }
  if (bz - az < INTERPOLATION_EPS_BEM3D) {
    bz += INTERPOLATION_EPS_BEM3D;
    az -= INTERPOLATION_EPS_BEM3D;
  }

  /*
   * Tschebyscheff points
   */

  cx = (bx + ax) * 0.5;
  dx = (bx - ax) * 0.5;
  cy = (by + ay) * 0.5;
  dy = (by - ay) * 0.5;
  cz = (bz + az) * 0.5;
  dz = (bz - az) * 0.5;

  index = 0;
  for (i = 0; i < m; ++i) {
    for (j = 0; j < m; ++j) {
      for (l = 0; l < m; ++l) {
	X[index][0] = cx + dx * x[i];
	X[index][1] = cy + dy * x[j];
	X[index][2] = cz + dz * x[l];
	index++;
      }
    }
  }

  assert(index == k);
}

static void
assemble_interpoints3d_avector(pcbem3d bem, pccluster t,
			       pavector px, pavector py, pavector pz)
{
  pcaprxbem3d aprx = bem->aprx;
  real     *x = aprx->x_inter;
  uint      m = aprx->m_inter;
  real      ax = t->bmin[0];
  real      bx = t->bmax[0];
  real      ay = t->bmin[1];
  real      by = t->bmax[1];
  real      az = t->bmin[2];
  real      bz = t->bmax[2];

  real      cx, dx, cy, dy, cz, dz;
  uint      i;

  if (bx - ax < INTERPOLATION_EPS_BEM3D) {
    bx += INTERPOLATION_EPS_BEM3D;
    ax -= INTERPOLATION_EPS_BEM3D;
  }
  if (by - ay < INTERPOLATION_EPS_BEM3D) {
    by += INTERPOLATION_EPS_BEM3D;
    ay -= INTERPOLATION_EPS_BEM3D;
  }
  if (bz - az < INTERPOLATION_EPS_BEM3D) {
    bz += INTERPOLATION_EPS_BEM3D;
    az -= INTERPOLATION_EPS_BEM3D;
  }

  /*
   * Tschebyscheff points
   */

  cx = (bx + ax) * 0.5;
  dx = (bx - ax) * 0.5;
  cy = (by + ay) * 0.5;
  dy = (by - ay) * 0.5;
  cz = (bz + az) * 0.5;
  dz = (bz - az) * 0.5;

  for (i = 0; i < m; ++i) {
    px->v[i] = cx + dx * x[i];
    py->v[i] = cy + dy * x[i];
    pz->v[i] = cz + dz * x[i];
  }
}

static void
assemble_bem3d_inter_row_rkmatrix(pccluster rc, uint rname,
				  pccluster cc, uint cname, pcbem3d bem,
				  prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  const uint rows = rc->size;
  const uint cols = cc->size;
  const uint m = aprx->m_inter;
  const uint k = aprx->k_inter;
  real(*z)[3];
  pavector  px, py, pz;

  (void) rname;
  (void) cname;

  z = (real(*)[3]) allocreal((size_t) (3 * k));
  px = new_avector(m);
  py = new_avector(m);
  pz = new_avector(m);

  assemble_interpoints3d_array(bem, rc, z);
  assemble_interpoints3d_avector(bem, rc, px, py, pz);

  resize_rkmatrix(R, rows, cols, k);

  kernels->kernel_col(cc->idx, (const real(*)[3]) z, bem, B);
  kernels->lagrange_row(rc->idx, px, py, pz, bem, A);

  del_avector(px);
  del_avector(py);
  del_avector(pz);
  freemem(z);
}

static void
assemble_bem3d_inter_col_rkmatrix(pccluster rc, uint rname,
				  pccluster cc, uint cname, pcbem3d bem,
				  prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  const uint rows = rc->size;
  const uint cols = cc->size;
  const uint m = aprx->m_inter;
  const uint k = aprx->k_inter;
  real(*z)[3];
  pavector  px, py, pz;

  (void) rname;
  (void) cname;

  z = (real(*)[3]) allocreal((size_t) (3 * k));
  px = new_avector(m);
  py = new_avector(m);
  pz = new_avector(m);

  assemble_interpoints3d_array(bem, cc, z);
  assemble_interpoints3d_avector(bem, cc, px, py, pz);

  resize_rkmatrix(R, rows, cols, k);

  kernels->fundamental_row(rc->idx, (const real(*)[3]) z, bem, A);
  kernels->lagrange_col(cc->idx, px, py, pz, bem, B);

  del_avector(px);
  del_avector(py);
  del_avector(pz);
  freemem(z);
}

static void
assemble_bem3d_inter_mixed_rkmatrix(pccluster rc, uint rname,
				    pccluster cc, uint cname, pcbem3d bem,
				    prkmatrix R)
{
  if (getdiam_2_cluster(rc) < getdiam_2_cluster(cc)) {
    assemble_bem3d_inter_row_rkmatrix(rc, rname, cc, cname, bem, R);
  }
  else {
    assemble_bem3d_inter_col_rkmatrix(rc, rname, cc, cname, bem, R);
  }
}

static void
assemble_bem3d_green_row_rkmatrix(pccluster rc, uint rname,
				  pccluster cc, uint cname, pcbem3d bem,
				  prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  real      delta = aprx->delta_green;
  real     *a = rc->bmin;
  real     *b = rc->bmax;

  pamatrix  T;
  real(*Z)[3], (*N)[3];
  real      diam;
  uint      nu, k2;

  (void) rname;
  (void) cname;

  k2 = 0.5 * aprx->k_green;
  diam =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(rc) : getdiam_2_cluster(rc);
  delta = delta * diam;

  T = new_amatrix(0, 0);

  resize_rkmatrix(R, rc->size, cc->size, 2 * k2);

  aprx->quadpoints(bem, a, b, delta, &Z, &N);

  init_sub_amatrix(T, A, rc->size, 0, k2, 0);
  kernels->fundamental_row(rc->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, B, cc->size, 0, k2, 0);
  kernels->dnz_kernel_col(cc->idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  for (nu = 0; nu < k2; ++nu) {
    N[nu][0] *= -1.0;
    N[nu][1] *= -1.0;
    N[nu][2] *= -1.0;
  }

  init_sub_amatrix(T, A, rc->size, 0, k2, k2);
  kernels->dnz_fundamental_row(rc->idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, B, cc->size, 0, k2, k2);
  kernels->kernel_col(cc->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  del_amatrix(T);
  freemem(Z);
  freemem(N);

}

static void
assemble_bem3d_green_col_rkmatrix(pccluster rc, uint rname,
				  pccluster cc, uint cname, pcbem3d bem,
				  prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  real      delta = aprx->delta_green;
  real     *a = cc->bmin;
  real     *b = cc->bmax;

  pamatrix  T;
  real(*Z)[3], (*N)[3];
  real      diam;
  uint      k2, nu;

  (void) rname;
  (void) cname;

  k2 = 0.5 * aprx->k_green;
  diam =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(cc) : getdiam_2_cluster(cc);
  delta = delta * diam;

  T = new_amatrix(0, 0);

  resize_rkmatrix(R, rc->size, cc->size, 2 * k2);

  aprx->quadpoints(bem, a, b, delta, &Z, &N);

  init_sub_amatrix(T, B, cc->size, 0, k2, 0);
  kernels->kernel_col(cc->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A, rc->size, 0, k2, 0);
  kernels->dnz_fundamental_row(rc->idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  for (nu = 0; nu < k2; ++nu) {
    N[nu][0] *= -1.0;
    N[nu][1] *= -1.0;
    N[nu][2] *= -1.0;
  }

  init_sub_amatrix(T, B, cc->size, 0, k2, k2);
  kernels->dnz_kernel_col(cc->idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A, rc->size, 0, k2, k2);
  kernels->fundamental_row(rc->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_green_mixed_rkmatrix(pccluster rc, uint rname,
				    pccluster cc, uint cname, pcbem3d bem,
				    prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  real      diamt, diams;

  diamt =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(rc) : getdiam_2_cluster(rc);
  diams =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(cc) : getdiam_2_cluster(cc);

  if (diamt < diams) {
    assemble_bem3d_green_row_rkmatrix(rc, rname, cc, cname, bem, R);
  }
  else {
    assemble_bem3d_green_col_rkmatrix(rc, rname, cc, cname, bem, R);
  }
}

static void
assemble_row_greencluster3d(pcbem3d bem, pgreencluster3d gc)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pccluster c = gc->t;
  uint      rows = c->size;
  real      eps = aprx->accur_aca;
  uint      rank = aprx->k_green;
  real      delta = aprx->delta_green;
  uint      k = aprx->k_green;
  real     *a = c->bmin;
  real     *b = c->bmax;

  prkmatrix R;
  pamatrix  A_t, RC, T;
  real(*Z)[3], (*N)[3];
  real      diam;
  uint     *xi;
  uint      i, k2;

  k2 = 0.5 * rank;
  diam =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(c) : getdiam_2_cluster(c);
  delta = delta * diam;

  A_t = new_amatrix(rows, k);

  T = new_amatrix(0, 0);

  aprx->quadpoints(bem, a, b, delta, &Z, &N);

  init_sub_amatrix(T, A_t, c->size, 0, k2, k2);
  kernels->dnz_fundamental_row(c->idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, c->size, 0, k2, 0);
  kernels->fundamental_row(c->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(rows, k, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;

  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &R->A, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  gc->xi = xi;
  gc->xihat = allocuint(rank);
  for (i = 0; i < rank; ++i) {
    gc->xihat[i] = c->idx[xi[i]];
  }

  resize_amatrix(gc->V, rows, rank);
  copy_amatrix(false, &R->A, gc->V);

  del_rkmatrix(R);
  del_amatrix(RC);
  del_amatrix(A_t);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_row_rkmatrix(pccluster rc, uint rname,
					pccluster cc, uint cname, pcbem3d bem,
					prkmatrix R)
{
  pparbem3d par = bem->par;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  uint      rows = rc->size;
  uint      cols = cc->size;

  pamatrix  V;
  pgreencluster3d grc;
  uint     *xihat;
  uint      rank;

  (void) cname;

  grc = par->grcn[rname];

#ifdef USE_OPENMP
#pragma omp critical
#endif
  if (grc == NULL) {
    grc = par->grcn[rname] = new_greencluster3d(rc);
    assemble_row_greencluster3d(bem, grc);
  }

  V = grc->V;
  rank = V->cols;
  xihat = grc->xihat;

  resize_rkmatrix(R, rows, cols, rank);

  copy_amatrix(false, V, A);
  bem->nearfield(xihat, cc->idx, bem, true, B);
}

static void
assemble_col_greencluster3d(pcbem3d bem, pgreencluster3d gc)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pccluster c = gc->t;
  uint      cols = c->size;
  real      eps = aprx->accur_aca;
  uint      rank = aprx->k_green;
  real      delta = aprx->delta_green;
  real     *a = c->bmin;
  real     *b = c->bmax;

  prkmatrix R;
  pamatrix  A_t, RC, T;
  real(*Z)[3], (*N)[3];
  uint     *xi;
  real      diam;
  uint      i, k2;

  k2 = 0.5 * rank;
  diam =
    (aprx->quadpoints == build_bem3d_cube_quadpoints) ?
    getdiam_max_cluster(c) : getdiam_2_cluster(c);
  delta = delta * diam;

  A_t = new_amatrix(cols, 2 * k2);
  T = new_amatrix(0, 0);

  aprx->quadpoints(bem, a, b, delta, &Z, &N);

  init_sub_amatrix(T, A_t, c->size, 0, k2, k2);
  kernels->dnz_kernel_col(c->idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, c->size, 0, k2, 0);
  kernels->kernel_col(c->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(cols, rank, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;

  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &R->A, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  gc->xi = xi;
  gc->xihat = allocuint(rank);
  for (i = 0; i < rank; ++i) {
    gc->xihat[i] = c->idx[xi[i]];
  }

  resize_amatrix(gc->V, cols, rank);
  copy_amatrix(false, &R->A, gc->V);

  del_rkmatrix(R);
  del_amatrix(RC);
  del_amatrix(A_t);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_col_rkmatrix(pccluster rc, uint rname,
					pccluster cc, uint cname, pcbem3d bem,
					prkmatrix R)
{
  pparbem3d par = bem->par;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  uint      rows = rc->size;
  uint      cols = cc->size;

  pamatrix  V;
  pgreencluster3d gcc;
  uint     *xihat;
  uint      rank;

  (void) rname;

  gcc = par->gccn[cname];

#ifdef USE_OPENMP
#pragma omp critical
#endif
  if (gcc == NULL) {
    gcc = par->gccn[cname] = new_greencluster3d(cc);
    assemble_col_greencluster3d(bem, gcc);
  }

  V = gcc->V;
  rank = V->cols;
  xihat = gcc->xihat;

  resize_amatrix(A, rows, rank);
  resize_amatrix(B, cols, rank);
  R->k = rank;

  bem->nearfield(rc->idx, xihat, bem, false, A);
  copy_amatrix(false, V, B);
}

static void
assemble_bem3d_greenhybrid_mixed_rkmatrix(pccluster rc, uint rname,
					  pccluster cc, uint cname,
					  pcbem3d bem, prkmatrix R)
{
  pparbem3d par = bem->par;
  pamatrix  A = &R->A;
  pamatrix  B = &R->B;
  uint      rows = rc->size;
  uint      cols = cc->size;

  pamatrix  V, W;
  pgreencluster3d grc, gcc;
  uint     *xihatV, *xihatW;
  uint      rankV, rankW;

  grc = par->grcn[rname];
  gcc = par->gccn[cname];

#ifdef USE_OPENMP
#pragma omp critical
#endif
  if (grc == NULL) {
    grc = par->grcn[rname] = new_greencluster3d(rc);
    assemble_row_greencluster3d(bem, grc);
  }

#ifdef USE_OPENMP
#pragma omp critical
#endif
  if (gcc == NULL) {
    gcc = par->gccn[cname] = new_greencluster3d(cc);
    assemble_col_greencluster3d(bem, gcc);
  }

  rankV = grc->V->cols;
  rankW = gcc->V->cols;

  if (cols * rankV <= rows * rankW) {
    V = grc->V;
    xihatV = grc->xihat;

    resize_amatrix(A, rows, rankV);
    resize_amatrix(B, cols, rankV);
    R->k = rankV;

    copy_amatrix(false, V, A);
    bem->nearfield(xihatV, cc->idx, bem, true, B);
  }
  else {
    W = gcc->V;
    xihatW = gcc->xihat;

    resize_amatrix(A, rows, rankW);
    resize_amatrix(B, cols, rankW);
    R->k = rankW;

    bem->nearfield(rc->idx, xihatW, bem, false, A);
    copy_amatrix(false, W, B);
  }

}

static void
assemble_bem3d_ACA_rkmatrix(pccluster rc, uint rname, pccluster cc,
			    uint cname, pcbem3d bem, prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  const real accur = aprx->accur_aca;
  const uint *ridx = rc->idx;
  const uint *cidx = cc->idx;
  const uint rows = rc->size;
  const uint cols = cc->size;

  pamatrix  G;

  (void) rname;
  (void) cname;

  G = new_amatrix(rows, cols);
  bem->nearfield(ridx, cidx, bem, false, G);

  decomp_fullaca_rkmatrix(G, accur, NULL, NULL, R);

  del_amatrix(G);
}

static void
assemble_bem3d_PACA_rkmatrix(pccluster rc, uint rname, pccluster cc,
			     uint cname, pcbem3d bem, prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  const real accur = aprx->accur_aca;
  void      (*entry) (const uint *, const uint *, void *, bool, pamatrix) =
    (void (*)(const uint *, const uint *, void *, bool, pamatrix)) bem->
    nearfield;
  const uint *ridx = rc->idx;
  const uint *cidx = cc->idx;
  const uint rows = rc->size;
  const uint cols = cc->size;

  (void) rname;
  (void) cname;

  decomp_partialaca_rkmatrix(entry, (void *) bem, ridx, rows, cidx, cols,
			     accur, NULL, NULL, R);
}

static void
assemble_bem3d_HCA_rkmatrix(pccluster rc, uint rname, pccluster cc,
			    uint cname, pcbem3d bem, prkmatrix R)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  const real accur = aprx->accur_aca;
  const uint k = aprx->k_inter;
  const uint *ridx = rc->idx;
  const uint *cidx = cc->idx;
  const uint rows = rc->size;
  const uint cols = cc->size;

  prkmatrix R2;
  pamatrix  S, C, D;
  real(*IT)[3], (*IS)[3], (*ITk)[3], (*ISk)[3];
  uint     *I_k, *J_k;
  uint      i, j, rank;

  (void) rname;
  (void) cname;

  IT = (real(*)[3]) allocreal(3 * k);
  IS = (real(*)[3]) allocreal(3 * k);

  assemble_interpoints3d_array(bem, rc, IT);
  assemble_interpoints3d_array(bem, cc, IS);

  S = new_amatrix(k, k);
  R2 = new_rkmatrix(k, k, 0);

  kernels->fundamental((const real(*)[3]) IT, (const real(*)[3]) IS, S);
  decomp_fullaca_rkmatrix(S, accur, &I_k, &J_k, R2);
  rank = R2->k;
  C = &R2->A;
  D = &R2->B;

  ITk = (real(*)[3]) allocreal(3 * rank);
  ISk = (real(*)[3]) allocreal(3 * rank);

  for (i = 0; i < rank; ++i) {
    for (j = 0; j < 3; ++j) {
      ITk[i][j] = IT[I_k[i]][j];
      ISk[i][j] = IS[J_k[i]][j];
    }
  }

  resize_amatrix(S, rank, rank);

  copy_lower_aca_amatrix(true, C, I_k, S);
  copy_upper_aca_amatrix(false, D, J_k, S);

  resize_rkmatrix(R, rows, cols, rank);

  kernels->kernel_row(ridx, (const real(*)[3]) ISk, bem, &R->A);
  kernels->kernel_col(cidx, (const real(*)[3]) ITk, bem, &R->B);

  if (rows < cols) {
    triangularsolve_amatrix(false, false, true, S, true, &R->A);
    triangularsolve_amatrix(true, true, true, S, true, &R->A);
  }
  else {
    triangularsolve_amatrix(true, true, false, S, true, &R->B);
    triangularsolve_amatrix(false, false, false, S, true, &R->B);
  }

  del_rkmatrix(R2);
  del_amatrix(S);
  freemem(IT);
  freemem(IS);
  freemem(ITk);
  freemem(ISk);
}

static void
assemble_bem3d_inter_row_clusterbasis(pcbem3d bem, pclusterbasis rb,
				      uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  V = &rb->V;
  pccluster t = rb->t;
  const uint m = aprx->m_inter;
  const uint k = aprx->k_inter;

  pavector  px, py, pz;

  (void) rname;

  px = new_avector(m);
  py = new_avector(m);
  pz = new_avector(m);

  assemble_interpoints3d_avector(bem, t, px, py, pz);

  resize_amatrix(V, t->size, k);
  rb->k = k;
  update_clusterbasis(rb);

  kernels->lagrange_row(t->idx, px, py, pz, bem, V);

  del_avector(px);
  del_avector(py);
  del_avector(pz);
}

static void
assemble_bem3d_inter_transfer_clusterbasis(pcbem3d bem,
					   pclusterbasis cb, uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pccluster t = cb->t;
  uint      sons = t->sons;
  const uint m = aprx->m_inter;
  const uint k = aprx->k_inter;

  pamatrix  E;
  real(*X)[3];
  pavector  px, py, pz;
  uint      s;

  (void) rname;

  X = (real(*)[3]) allocmem(3 * k * sizeof(real));
  px = new_avector(m);
  py = new_avector(m);
  pz = new_avector(m);

  assemble_interpoints3d_avector(bem, t, px, py, pz);

  resize_clusterbasis(cb, k);

  for (s = 0; s < sons; ++s) {
    E = &cb->son[s]->E;

    assemble_interpoints3d_array(bem, cb->son[s]->t, X);

    assemble_bem3d_lagrange_amatrix((const real(*)[3]) X, px, py, pz, E);
  }

  del_avector(px);
  del_avector(py);
  del_avector(pz);
  freemem(X);
}

static void
assemble_bem3d_inter_col_clusterbasis(pcbem3d bem, pclusterbasis cb,
				      uint cname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pamatrix  V = &cb->V;
  pccluster t = cb->t;
  const uint m = aprx->m_inter;
  const uint k = aprx->k_inter;

  pavector  px, py, pz;

  (void) cname;

  px = new_avector(m);
  py = new_avector(m);
  pz = new_avector(m);

  assemble_interpoints3d_avector(bem, t, px, py, pz);

  resize_amatrix(V, t->size, k);
  cb->k = k;
  update_clusterbasis(cb);

  kernels->lagrange_col(t->idx, px, py, pz, bem, V);

  del_avector(px);
  del_avector(py);
  del_avector(pz);
}

static void
assemble_bem3d_inter_uniform(uint rname, uint cname, pcbem3d bem, puniform U)
{
  pkernelbem3d kernels = bem->kernels;
  pccluster rc = U->rb->t;
  pccluster cc = U->cb->t;
  const uint kr = U->rb->k;
  const uint kc = U->cb->k;
  pamatrix  S = &U->S;

  real(*xi_r)[3], (*xi_c)[3];

  (void) rname;
  (void) cname;

  resize_amatrix(S, kr, kc);

  xi_r = (real(*)[3]) allocreal(3 * kr);
  xi_c = (real(*)[3]) allocreal(3 * kc);

  assemble_interpoints3d_array(bem, rc, xi_r);
  assemble_interpoints3d_array(bem, cc, xi_c);

  kernels->fundamental((const real(*)[3]) xi_r, (const real(*)[3]) xi_c, S);

  freemem(xi_r);
  freemem(xi_c);
}

static void
update_pivotelements_greenclusterbasis3d(pgreenclusterbasis3d * grbn,
					 pcclusterbasis cb, uint * I_t,
					 uint k)
{
  uint      sons = cb->sons;
  pgreenclusterbasis3d grb = *grbn;

  pgreenclusterbasis3d grb1;
  uint      i, j, rname1, size, rank;

  uint     *xi;

  if (sons > 0) {
    xi = allocuint(k);

    for (j = 0; j < k; ++j) {
      rname1 = 1;
      grb1 = grbn[rname1];
      size = 0;
      rank = 0;
      for (i = 0; i < sons; ++i) {
	grb1 = grbn[rname1];
	if (I_t[j] < rank + cb->son[i]->k) {
	  break;
	}

	rname1 += cb->son[i]->t->desc;
	rank += cb->son[i]->k;
	size += cb->son[i]->t->size;
      }
      xi[j] = grb1->xi[I_t[j] - rank] + size;
    }
  }
  else {
    xi = I_t;
  }

  grb->xi = xi;
  grb->xihat = allocuint(k);
  for (i = 0; i < k; ++i) {
    grb->xihat[i] = cb->t->idx[xi[i]];
  }
}

static uint *
collect_pivotelements_greenclusterbasis3d(pgreenclusterbasis3d * grbn,
					  uint * rank)
{
  pcclusterbasis cb = (*grbn)->cb;
  uint      sons = cb->sons;

  pgreenclusterbasis3d grb1;
  uint      size, rname1, i, j, r;

  uint     *idx;

  r = 0;
  for (i = 0; i < sons; ++i) {
    assert(cb->son[i]->k > 0);
    r += cb->son[i]->k;
  }

  idx = allocuint(r);

  r = 0;
  size = 0;
  rname1 = 1;
  for (i = 0; i < sons; ++i) {
    grb1 = grbn[rname1];
    for (j = 0; j < cb->son[i]->k; ++j) {
      idx[r + j] = cb->t->idx[size + grb1->xi[j]];
    }
    r += cb->son[i]->k;
    size += cb->son[i]->t->size;
    rname1 += cb->son[i]->t->desc;
  }

  *rank = r;

  return idx;
}

static void
assemble_bem3d_greenhybrid_leaf_row_clusterbasis(pcbem3d bem,
						 pclusterbasis rb, uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster c = rb->t;
  const uint rows = c->size;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;

  prkmatrix R;
  pamatrix  A_t, T, RC;
  pgreenclusterbasis3d grb;
  real(*Z)[3], (*N)[3];
  uint     *xi;
  uint      k2;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(c) : getdiam_2_cluster(c));

  aprx->quadpoints(bem, c->bmin, c->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  grb = par->grbn[rname];
  if (grb == NULL) {
    grb = par->grbn[rname] = new_greenclusterbasis3d(rb);
  }

  A_t = new_amatrix(rows, rank);

  init_sub_amatrix(T, A_t, rows, 0, k2, 0);
  kernels->fundamental_row(c->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rows, 0, k2, k2);
  kernels->dnz_fundamental_row(c->idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(rows, rank, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;
  resize_amatrix(&rb->V, rows, rank);
  copy_amatrix(false, &R->A, &rb->V);

  rb->k = rank;
  update_clusterbasis(rb);

  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &rb->V, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &rb->V);

  update_pivotelements_greenclusterbasis3d(par->grbn + rname, rb, xi, rank);

  del_rkmatrix(R);
  del_amatrix(A_t);
  del_amatrix(RC);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_transfer_row_clusterbasis(pcbem3d bem,
						     pclusterbasis rb,
						     uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster c = rb->t;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;
  uint      sons = c->sons;

  prkmatrix R;
  pamatrix  A_t, T, RC, E;
  pgreenclusterbasis3d grb;
  real(*Z)[3], (*N)[3];
  uint     *I_t, *idx;
  uint      i, k2, newrank;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(c) : getdiam_2_cluster(c));

  aprx->quadpoints(bem, c->bmin, c->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  grb = par->grbn[rname];
  if (grb == NULL) {
    grb = par->grbn[rname] = new_greenclusterbasis3d(rb);
  }

  idx = collect_pivotelements_greenclusterbasis3d(par->grbn + rname, &rank);

  A_t = new_amatrix(rank, 2 * k2);

  init_sub_amatrix(T, A_t, rank, 0, k2, 0);
  kernels->fundamental_row(idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rank, 0, k2, k2);
  kernels->dnz_fundamental_row(idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(rank, 2 * k2, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &I_t, NULL, R);
  newrank = R->k;

  RC = new_amatrix(newrank, newrank);
  copy_lower_aca_amatrix(true, &R->A, I_t, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  resize_clusterbasis(rb, newrank);

  rank = 0;
  for (i = 0; i < sons; ++i) {
    E = &rb->son[i]->E;
    init_sub_amatrix(T, &R->A, rb->son[i]->k, rank, newrank, 0);
    copy_amatrix(false, T, E);
    uninit_amatrix(T);
    rank += rb->son[i]->k;
  }

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->grbn + rname, rb, I_t,
					   newrank);

  freemem(idx);
  freemem(I_t);
  del_rkmatrix(R);
  del_amatrix(RC);
  ;
  del_amatrix(A_t);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_leaf_col_clusterbasis(pcbem3d bem,
						 pclusterbasis cb, uint cname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster c = cb->t;
  const uint rows = c->size;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;

  prkmatrix R;
  pamatrix  A_t, T, RC;
  pgreenclusterbasis3d gcb;
  real(*Z)[3], (*N)[3];
  uint     *xi;
  uint      k2;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(c) : getdiam_2_cluster(c));

  aprx->quadpoints(bem, c->bmin, c->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  gcb = par->gcbn[cname];
  if (gcb == NULL) {
    gcb = par->gcbn[cname] = new_greenclusterbasis3d(cb);
  }

  A_t = new_amatrix(rows, rank);

  init_sub_amatrix(T, A_t, rows, 0, k2, 0);
  kernels->kernel_col(c->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rows, 0, k2, k2);
  kernels->dnz_kernel_col(c->idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(rows, rank, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;
  resize_amatrix(&cb->V, rows, rank);
  copy_amatrix(false, &R->A, &cb->V);

  cb->k = rank;
  update_clusterbasis(cb);

  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &cb->V, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &cb->V);

  update_pivotelements_greenclusterbasis3d(par->gcbn + cname, cb, xi, rank);

  del_rkmatrix(R);
  del_amatrix(A_t);
  del_amatrix(RC);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_transfer_col_clusterbasis(pcbem3d bem,
						     pclusterbasis cb,
						     uint cname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster c = cb->t;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;
  uint      sons = c->sons;

  prkmatrix R;
  pamatrix  A_t, T, RC, E;
  pgreenclusterbasis3d gcb;
  real(*Z)[3], (*N)[3];
  uint     *I_t, *idx;
  uint      i, k2, newrank;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(c) : getdiam_2_cluster(c));

  aprx->quadpoints(bem, c->bmin, c->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  gcb = par->gcbn[cname];
  if (gcb == NULL) {
    gcb = par->gcbn[cname] = new_greenclusterbasis3d(cb);
  }

  idx = collect_pivotelements_greenclusterbasis3d(par->gcbn + cname, &rank);

  A_t = new_amatrix(rank, 2 * k2);

  init_sub_amatrix(T, A_t, rank, 0, k2, 0);
  kernels->kernel_col(idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rank, 0, k2, k2);
  kernels->dnz_kernel_col(idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  R = new_rkmatrix(rank, 2 * k2, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &I_t, NULL, R);
  newrank = R->k;

  RC = new_amatrix(newrank, newrank);
  copy_lower_aca_amatrix(true, &R->A, I_t, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  resize_clusterbasis(cb, newrank);

  rank = 0;
  for (i = 0; i < sons; ++i) {
    E = &cb->son[i]->E;
    init_sub_amatrix(T, &R->A, cb->son[i]->k, rank, newrank, 0);
    copy_amatrix(false, T, E);
    uninit_amatrix(T);
    rank += cb->son[i]->k;
  }

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->gcbn + cname, cb, I_t,
					   newrank);

  freemem(I_t);
  freemem(idx);
  del_rkmatrix(R);
  del_amatrix(RC);
  del_amatrix(A_t);
  del_amatrix(T);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybrid_uniform(uint rname, uint cname,
				   pcbem3d bem, puniform U)
{
  pparbem3d par = bem->par;
  const uint kr = U->rb->k;
  const uint kc = U->cb->k;
  pamatrix  S = &U->S;

  pgreenclusterbasis3d grb, gcb;
  uint     *xihatV, *xihatW;

  grb = par->grbn[rname];
  gcb = par->gcbn[cname];

  assert(grb != NULL);
  assert(gcb != NULL);

  xihatV = grb->xihat;
  xihatW = gcb->xihat;

  resize_amatrix(S, kr, kc);

  bem->nearfield(xihatV, xihatW, bem, false, S);
}

static void
assemble_bem3d_greenhybridortho_leaf_row_clusterbasis(pcbem3d bem,
						      pclusterbasis rb,
						      uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster t = rb->t;
  pamatrix  V = &rb->V;
  const uint rows = t->size;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;

  prkmatrix R;
  pamatrix  A_t, T, RC;
  pavector  tau;
  pgreenclusterbasis3d grb;
  real(*Z)[3], (*N)[3];
  uint     *xi;
  uint      k2;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(t) : getdiam_2_cluster(t));

  aprx->quadpoints(bem, t->bmin, t->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  grb = par->grbn[rname];
  if (grb == NULL) {
    grb = par->grbn[rname] = new_greenclusterbasis3d(rb);
  }

  /* setup up A_t from green's formula. */
  A_t = new_amatrix(rows, rank);

  init_sub_amatrix(T, A_t, rows, 0, k2, 0);
  kernels->fundamental_row(t->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rows, 0, k2, k2);
  kernels->dnz_fundamental_row(t->idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  /* Do ACA of A_t to obtain row pivots. */
  R = new_rkmatrix(rows, rank, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;
  assert(rows >= rank);

  resize_clusterbasis(rb, rank);

  /* Compute \hat V_t = C_t * (R_t * C_t)^-1. */
  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &R->A, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  /* Compute QR factorization of \hat V_t. */
  tau = new_avector(rank);
  resize_amatrix(V, rows, rank);
  grb->Qinv = new_amatrix(rank, rank);

  qrdecomp_amatrix(&R->A, tau);
  qrexpand_amatrix(&R->A, tau, V);
  copy_upper_amatrix(&R->A, false, grb->Qinv);

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->grbn + rname, rb, xi, rank);

  /* clean up */
  del_rkmatrix(R);
  del_amatrix(A_t);
  del_amatrix(RC);
  del_amatrix(T);
  del_avector(tau);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybridortho_transfer_row_clusterbasis(pcbem3d bem,
							  pclusterbasis rb,
							  uint rname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster t = rb->t;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;
  uint      sons = t->sons;

  prkmatrix R;
  pamatrix  A_t, T, RC, E, Q;
  pavector  tau;
  pgreenclusterbasis3d grb, grb1;
  real(*Z)[3], (*N)[3];
  uint     *I_t, *idx;
  uint      i, k2, newrank, rname1;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(t) : getdiam_2_cluster(t));

  aprx->quadpoints(bem, t->bmin, t->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  grb = par->grbn[rname];
  if (grb == NULL) {
    grb = par->grbn[rname] = new_greenclusterbasis3d(rb);
  }

  idx = collect_pivotelements_greenclusterbasis3d(par->grbn + rname, &rank);

  /* setup up A_t from green's formula with reduced row indices. */
  A_t = new_amatrix(rank, 2 * k2);

  init_sub_amatrix(T, A_t, rank, 0, k2, 0);
  kernels->fundamental_row(idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rank, 0, k2, k2);
  kernels->dnz_fundamental_row(idx, (const real(*)[3]) Z,
			       (const real(*)[3]) N, (pcbem3d) bem, T);
  uninit_amatrix(T);

  /* Do ACA of \tilde A_t. */
  R = new_rkmatrix(rank, 2 * k2, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &I_t, NULL, R);
  newrank = R->k;
  assert(rank >= newrank);
  resize_clusterbasis(rb, newrank);

  /* Compute \hat V_t = C_t * (R_t * C_t)^-1. */
  RC = new_amatrix(newrank, newrank);
  copy_lower_aca_amatrix(true, &R->A, I_t, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  /* Multiply with triangular matrices of the sons from the left. */
  rank = 0;
  rname1 = rname + 1;
  for (i = 0; i < sons; ++i) {
    grb1 = par->grbn[rname1];
    init_sub_amatrix(T, &R->A, rb->son[i]->k, rank, newrank, 0);
    triangulareval_amatrix(false, false, false, grb1->Qinv, false, T);
    uninit_amatrix(T);
    rank += rb->son[i]->k;
    rname1 += grb1->cb->t->desc;
  }

  /* Compute QR factorization of \hat V_t. */
  tau = new_avector(newrank);
  Q = new_amatrix(rank, newrank);
  grb->Qinv = new_amatrix(newrank, newrank);

  qrdecomp_amatrix(&R->A, tau);
  qrexpand_amatrix(&R->A, tau, Q);
  copy_upper_amatrix(&R->A, false, grb->Qinv);

  /* copy transfer matrices to their location. */
  rank = 0;
  for (i = 0; i < sons; ++i) {
    E = &rb->son[i]->E;
    init_sub_amatrix(T, Q, rb->son[i]->k, rank, newrank, 0);
    copy_amatrix(false, T, E);
    uninit_amatrix(T);
    rank += rb->son[i]->k;
  }

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->grbn + rname, rb, I_t,
					   newrank);

  /* clean up */
  del_rkmatrix(R);
  del_amatrix(RC);
  del_amatrix(A_t);
  del_amatrix(T);
  del_amatrix(Q);
  del_avector(tau);
  freemem(Z);
  freemem(N);
  freemem(idx);
  freemem(I_t);
}

static void
assemble_bem3d_greenhybridortho_leaf_col_clusterbasis(pcbem3d bem,
						      pclusterbasis cb,
						      uint cname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster t = cb->t;
  pamatrix  V = &cb->V;
  const uint rows = t->size;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;

  prkmatrix R;
  pamatrix  A_t, T, RC;
  pavector  tau;
  pgreenclusterbasis3d gcb;
  real(*Z)[3], (*N)[3];
  uint     *xi;
  uint      k2;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(t) : getdiam_2_cluster(t));

  aprx->quadpoints(bem, t->bmin, t->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  gcb = par->gcbn[cname];
  if (gcb == NULL) {
    gcb = par->gcbn[cname] = new_greenclusterbasis3d(cb);
  }

  /* setup up A_t from green's formula. */
  A_t = new_amatrix(rows, rank);

  init_sub_amatrix(T, A_t, rows, 0, k2, 0);
  kernels->kernel_col(t->idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rows, 0, k2, k2);
  kernels->dnz_kernel_col(t->idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  /* Do ACA of A_t to obtain row pivots. */
  R = new_rkmatrix(rows, rank, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &xi, NULL, R);
  rank = R->k;
  assert(rows >= rank);
  resize_clusterbasis(cb, rank);

  /* Compute \hat V_t = C_t * (R_t * C_t)^-1. */
  RC = new_amatrix(rank, rank);
  copy_lower_aca_amatrix(true, &R->A, xi, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  /* Compute QR factorization of \hat V_t. */
  tau = new_avector(rank);
  resize_amatrix(V, rows, rank);
  gcb->Qinv = new_amatrix(rank, rank);

  qrdecomp_amatrix(&R->A, tau);
  qrexpand_amatrix(&R->A, tau, V);
  copy_upper_amatrix(&R->A, false, gcb->Qinv);

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->gcbn + cname, cb, xi, rank);

  /* clean up */
  del_rkmatrix(R);
  del_amatrix(A_t);
  del_amatrix(RC);
  del_amatrix(T);
  del_avector(tau);
  freemem(Z);
  freemem(N);
}

static void
assemble_bem3d_greenhybridortho_transfer_col_clusterbasis(pcbem3d bem,
							  pclusterbasis cb,
							  uint cname)
{
  paprxbem3d aprx = bem->aprx;
  pkernelbem3d kernels = bem->kernels;
  pparbem3d par = bem->par;
  pccluster t = cb->t;
  const real eps = aprx->accur_aca;
  real      delta = aprx->delta_green;
  uint      rank = aprx->k_green;
  uint      sons = t->sons;

  prkmatrix R;
  pamatrix  A_t, T, RC, E, Q;
  pavector  tau;
  pgreenclusterbasis3d gcb, gcb1;
  real(*Z)[3], (*N)[3];
  uint     *I_t, *idx;
  uint      i, k2, newrank, cname1;

  k2 = rank * 0.5;
  delta = aprx->delta_green
    * ((aprx->quadpoints == build_bem3d_cube_quadpoints) ?
       getdiam_max_cluster(t) : getdiam_2_cluster(t));

  aprx->quadpoints(bem, t->bmin, t->bmax, delta, &Z, &N);

  T = new_amatrix(0, 0);

  gcb = par->gcbn[cname];
  if (gcb == NULL) {
    gcb = par->gcbn[cname] = new_greenclusterbasis3d(cb);
  }

  idx = collect_pivotelements_greenclusterbasis3d(par->gcbn + cname, &rank);

  /* setup up A_t from green's formula with reduced row indices. */
  A_t = new_amatrix(rank, 2 * k2);

  init_sub_amatrix(T, A_t, rank, 0, k2, 0);
  kernels->kernel_col(idx, (const real(*)[3]) Z, (pcbem3d) bem, T);
  uninit_amatrix(T);

  init_sub_amatrix(T, A_t, rank, 0, k2, k2);
  kernels->dnz_kernel_col(idx, (const real(*)[3]) Z, (const real(*)[3]) N,
			  (pcbem3d) bem, T);
  uninit_amatrix(T);

  /* Do ACA of \tilde A_t. */
  R = new_rkmatrix(rank, 2 * k2, 0);
  decomp_fullaca_rkmatrix(A_t, eps, &I_t, NULL, R);
  newrank = R->k;
  assert(rank >= newrank);
  resize_clusterbasis(cb, newrank);

  /* Compute \hat V_t = C_t * (R_t * C_t)^-1. */
  RC = new_amatrix(newrank, newrank);
  copy_lower_aca_amatrix(true, &R->A, I_t, RC);
  triangularsolve_amatrix(true, true, true, RC, true, &R->A);

  rank = 0;
  cname1 = cname + 1;
  for (i = 0; i < sons; ++i) {
    gcb1 = par->gcbn[cname1];
    init_sub_amatrix(T, &R->A, cb->son[i]->k, rank, newrank, 0);
    triangulareval_amatrix(false, false, false, gcb1->Qinv, false, T);
    uninit_amatrix(T);
    rank += cb->son[i]->k;
    cname1 += gcb1->cb->t->desc;
  }

  /* Compute QR factorization of \hat V_t. */
  tau = new_avector(newrank);
  Q = new_amatrix(rank, newrank);
  gcb->Qinv = new_amatrix(newrank, newrank);

  qrdecomp_amatrix(&R->A, tau);
  qrexpand_amatrix(&R->A, tau, Q);
  copy_upper_amatrix(&R->A, false, gcb->Qinv);

  /* copy transfer matrices to their location. */
  rank = 0;
  for (i = 0; i < sons; ++i) {
    E = &cb->son[i]->E;
    init_sub_amatrix(T, Q, cb->son[i]->k, rank, newrank, 0);
    copy_amatrix(false, T, E);
    uninit_amatrix(T);
    rank += cb->son[i]->k;
  }

  /* save local and global row pivot indices. */
  update_pivotelements_greenclusterbasis3d(par->gcbn + cname, cb, I_t,
					   newrank);

  /* clean up */
  del_rkmatrix(R);
  del_amatrix(RC);
  del_amatrix(A_t);
  del_amatrix(T);
  del_amatrix(Q);
  del_avector(tau);
  freemem(Z);
  freemem(N);
  freemem(idx);
  freemem(I_t);
}

static void
assemble_bem3d_greenhybridortho_uniform(uint rname, uint cname,
					pcbem3d bem, puniform U)
{
  pparbem3d par = bem->par;
  const uint kr = U->rb->k;
  const uint kc = U->cb->k;
  pamatrix  S = &U->S;

  pgreenclusterbasis3d grb, gcb;
  uint     *xihatV, *xihatW;

  grb = par->grbn[rname];
  gcb = par->gcbn[cname];

  assert(grb != NULL);
  assert(gcb != NULL);

  xihatV = grb->xihat;
  xihatW = gcb->xihat;

  resize_amatrix(S, kr, kc);

  bem->nearfield(xihatV, xihatW, bem, false, S);

  triangulareval_amatrix(false, false, false, grb->Qinv, false, S);
  triangulareval_amatrix(false, false, false, gcb->Qinv, true, S);
}

/* ------------------------------------------------------------
 lagrange-polynomials
 ------------------------------------------------------------ */

void
assemble_bem3d_lagrange_amatrix(const real(*X)[3], pcavector px,
				pcavector py, pcavector pz, pamatrix V)
{

  const uint rows = V->rows;
  const uint cols = V->cols;
  const uint ld = V->ld;
  const uint mx = px->dim;
  const uint my = py->dim;
  const uint mz = pz->dim;

  real     *denomx, *denomy, *denomz;
  uint      jx, jy, jz, i, l, index;
  real      lagr, denom;

  denomx = allocreal(mx);
  denomy = allocreal(my);
  denomz = allocreal(mz);

  /*
   * Eval Lagrange polynomials at points X
   */

  assert(mx * my * mz == cols);

  for (jx = 0; jx < mx; ++jx) {
    denom = 1.0;
    for (l = 0; l < jx; ++l) {
      denom *= (px->v[jx] - px->v[l]);
    }
    for (l = jx + 1; l < mx; ++l) {
      denom *= (px->v[jx] - px->v[l]);
    }
    denomx[jx] = 1.0 / denom;
  }

  for (jy = 0; jy < my; ++jy) {
    denom = 1.0;
    for (l = 0; l < jy; ++l) {
      denom *= (py->v[jy] - py->v[l]);
    }
    for (l = jy + 1; l < my; ++l) {
      denom *= (py->v[jy] - py->v[l]);
    }
    denomy[jy] = 1.0 / denom;
  }

  for (jz = 0; jz < mz; ++jz) {
    denom = 1.0;
    for (l = 0; l < jz; ++l) {
      denom *= (pz->v[jz] - pz->v[l]);
    }
    for (l = jz + 1; l < mz; ++l) {
      denom *= (pz->v[jz] - pz->v[l]);
    }
    denomz[jz] = 1.0 / denom;
  }

  index = 0;
  for (jx = 0; jx < mx; ++jx) {
    for (jy = 0; jy < my; ++jy) {
      for (jz = 0; jz < mz; ++jz) {
	denom = denomx[jx] * denomy[jy] * denomz[jz];
	for (i = 0; i < rows; ++i) {

	  lagr = 1.0;

	  for (l = 0; l < jx; ++l) {
	    lagr *= (X[i][0] - px->v[l]);
	  }
	  for (l = jx + 1; l < mx; ++l) {
	    lagr *= (X[i][0] - px->v[l]);
	  }

	  for (l = 0; l < jy; ++l) {
	    lagr *= (X[i][1] - py->v[l]);
	  }
	  for (l = jy + 1; l < my; ++l) {
	    lagr *= (X[i][1] - py->v[l]);
	  }

	  for (l = 0; l < jz; ++l) {
	    lagr *= (X[i][2] - pz->v[l]);
	  }
	  for (l = jz + 1; l < my; ++l) {
	    lagr *= (X[i][2] - pz->v[l]);
	  }

	  V->a[i + index * ld] = lagr * denom;
	}
	index++;
      }
    }
  }

  freemem(denomx);
  freemem(denomy);
  freemem(denomz);
}

void
assemble_bem3d_lagrange_const_amatrix(const uint * idx, pcavector px,
				      pcavector py, pcavector pz, pcbem3d bem,
				      pamatrix V)
{

  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const preal gr_g = (const preal) gr->g;
  uint      rows = V->rows;
  uint      ld = V->ld;
  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single + 3 * nq;
  uint      mx = px->dim;
  uint      my = py->dim;
  uint      mz = pz->dim;

  const real *A, *B, *C;
  real     *denomx, *denomy, *denomz;
  uint      t, tt, jx, jy, jz, q, l, index;
  real      gt, sum, lagr, denom, x, y, z, tx, sx, Ax, Bx, Cx;

  denomx = allocreal(mx);
  denomy = allocreal(my);
  denomz = allocreal(mz);

  /*
   * integrate Lagrange polynomials with constant basisfunctions
   */

  for (jx = 0; jx < mx; ++jx) {
    denom = 1.0;
    for (l = 0; l < jx; ++l) {
      denom *= (px->v[jx] - px->v[l]);
    }
    for (l = jx + 1; l < mx; ++l) {
      denom *= (px->v[jx] - px->v[l]);
    }
    denomx[jx] = 1.0 / denom;
  }

  for (jy = 0; jy < my; ++jy) {
    denom = 1.0;
    for (l = 0; l < jy; ++l) {
      denom *= (py->v[jy] - py->v[l]);
    }
    for (l = jy + 1; l < my; ++l) {
      denom *= (py->v[jy] - py->v[l]);
    }
    denomy[jy] = 1.0 / denom;
  }

  for (jz = 0; jz < mz; ++jz) {
    denom = 1.0;
    for (l = 0; l < jz; ++l) {
      denom *= (pz->v[jz] - pz->v[l]);
    }
    for (l = jz + 1; l < mz; ++l) {
      denom *= (pz->v[jz] - pz->v[l]);
    }
    denomz[jz] = 1.0 / denom;
  }

  index = 0;
  for (jx = 0; jx < mx; ++jx) {
    for (jy = 0; jy < my; ++jy) {
      for (jz = 0; jz < mz; ++jz) {
	denom = denomx[jx] * denomy[jy] * denomz[jz];
	for (t = 0; t < rows; ++t) {
	  tt = (idx == NULL ? t : idx[t]);
	  gt = gr_g[tt];
	  A = gr_x[gr_t[tt][0]];
	  B = gr_x[gr_t[tt][1]];
	  C = gr_x[gr_t[tt][2]];

	  sum = 0.0;

	  for (q = 0; q < nq; ++q) {
	    tx = xx[q];
	    sx = yy[q];
	    Ax = 1.0 - tx;
	    Bx = tx - sx;
	    Cx = sx;

	    x = A[0] * Ax + B[0] * Bx + C[0] * Cx;
	    y = A[1] * Ax + B[1] * Bx + C[1] * Cx;
	    z = A[2] * Ax + B[2] * Bx + C[2] * Cx;

	    lagr = 1.0;

	    for (l = 0; l < jx; ++l) {
	      lagr *= (x - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagr *= (x - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagr *= (y - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagr *= (y - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagr *= (z - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagr *= (z - pz->v[l]);
	    }

	    sum += ww[q] * lagr;

	  }
	  V->a[t + index * ld] = gt * sum * denom;
	}
	index++;
      }
    }
  }

  freemem(denomx);
  freemem(denomy);
  freemem(denomz);
}

void
assemble_bem3d_lagrange_linear_amatrix(const uint * idx, pcavector px,
				       pcavector py, pcavector pz,
				       pcbem3d bem, pamatrix V)
{
  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const preal gr_g = (const preal) gr->g;
  plistnode *v2t = bem->v2t;
  uint      rows = V->rows;
  field    *aa = V->a;
  uint      ld = V->ld;
  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single;
  real      base = bem->sq->base_single;
  field    *quad;
  uint      mx = px->dim;
  uint      my = py->dim;
  uint      mz = pz->dim;

  ptri_list tl, tl1;
  pvert_list vl;
  const real *A, *B, *C;
  uint      tri_sp[3];
  plistnode v;
  uint      s, ss, i, jx, jy, jz, l, k, q, cj, index;
  real      gs, sum, lagr, x, y, z, tx, sx, Ax, Bx, Cx;
  longindex ii, vv;

  quad = allocfield(bem->sq->n_single);

  clear_amatrix(V);

  tl = NULL;

  cj = 0;
  for (i = 0; i < rows; ++i) {
    ii = (idx == NULL ? i : idx[i]);
    for (v = v2t[ii], vv = v->data; v->next != NULL;
	 v = v->next, vv = v->data) {

      tl1 = tl;
      while (tl1 && tl1->t != vv) {
	tl1 = tl1->next;
      }

      if (tl1 == NULL) {
	tl1 = tl = new_tri_list(tl);
	tl->t = vv;
	cj++;
      }

      tl1->vl = new_vert_list(tl1->vl);
      tl1->vl->v = i;
    }
  }

  for (s = 0, tl1 = tl; s < cj; s++, tl1 = tl1->next) {
    ss = tl1->t;
    gs = gr_g[ss];
    A = gr_x[gr_t[ss][0]];
    B = gr_x[gr_t[ss][1]];
    C = gr_x[gr_t[ss][2]];

    for (i = 0; i < 3; ++i) {
      tri_sp[i] = gr_t[ss][i];
    }

    index = 0;

    for (jx = 0; jx < mx; ++jx) {
      for (jy = 0; jy < my; ++jy) {
	for (jz = 0; jz < mz; ++jz) {

	  for (q = 0; q < nq; ++q) {
	    tx = xx[q];
	    sx = yy[q];
	    Ax = 1.0 - tx;
	    Bx = tx - sx;
	    Cx = sx;

	    x = A[0] * Ax + B[0] * Bx + C[0] * Cx;
	    y = A[1] * Ax + B[1] * Bx + C[1] * Cx;
	    z = A[2] * Ax + B[2] * Bx + C[2] * Cx;

	    lagr = 1.0;

	    /* TODO optimize lagrange polynomial eval */
	    for (l = 0; l < jx; ++l) {
	      lagr *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagr *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagr *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagr *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagr *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagr *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }

	    quad[q] = lagr;
	  }

	  ww = bem->sq->w_single;
	  vl = tl1->vl;
	  while (vl) {
	    k = vl->v;
	    if (k < rows) {
	      ii = idx == NULL ? k : idx[k];
	      for (i = 0; i < 3; ++i) {
		if (ii == tri_sp[i]) {
		  sum = base;

		  for (q = 0; q < nq; ++q) {
		    sum += ww[q] * quad[q];
		  }

		  aa[k + index * ld] += sum * gs;
		}
		ww += nq;
	      }
	      ww = bem->sq->w_single;
	    }
	    vl = vl->next;
	  }

	  index++;
	}
      }
    }
  }

  del_tri_list(tl);
  freemem(quad);
}

void
assemble_bem3d_dn_lagrange_const_amatrix(const uint * idx, pcavector px,
					 pcavector py, pcavector pz,
					 pcbem3d bem, pamatrix V)
{
  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const     real(*gr_n)[3] = (const real(*)[3]) gr->n;
  const preal gr_g = (const preal) gr->g;
  uint      rows = V->rows;
  uint      ld = V->ld;

  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single + 3 * nq;
  uint      mx = px->dim;
  uint      my = py->dim;
  uint      mz = pz->dim;

  const real *A, *B, *C, *nt;
  real      lagr[3];
  uint      t, tt, jx, jy, jz, q, l, index;
  real      gt, sum, lagrx, lagry, lagrz, x, y, z, tx, sx, Ax, Bx, Cx;

  /*
   * integrate Lagrange polynomials with constant basisfunctions
   */

  for (t = 0; t < rows; ++t) {
    tt = (idx == NULL ? t : idx[t]);
    gt = gr_g[tt];
    nt = gr_n[tt];
    A = gr_x[gr_t[tt][0]];
    B = gr_x[gr_t[tt][1]];
    C = gr_x[gr_t[tt][2]];

    index = 0;

    for (jx = 0; jx < mx; ++jx) {
      for (jy = 0; jy < my; ++jy) {
	for (jz = 0; jz < mz; ++jz) {

	  sum = 0.0;

	  for (q = 0; q < nq; ++q) {
	    tx = xx[q];
	    sx = yy[q];
	    Ax = 1.0 - tx;
	    Bx = tx - sx;
	    Cx = sx;

	    x = A[0] * Ax + B[0] * Bx + C[0] * Cx;
	    y = A[1] * Ax + B[1] * Bx + C[1] * Cx;
	    z = A[2] * Ax + B[2] * Bx + C[2] * Cx;

	    lagrx = 1.0;
	    lagry = 1.0;
	    lagrz = 1.0;

	    /* TODO optimize lagrange polynomial eval */
	    for (l = 0; l < jx; ++l) {
	      lagrx *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagrx *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagry *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagry *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagrz *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagrz *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }

	    lagr[0] = 0.0;
	    lagr[1] = 0.0;
	    lagr[2] = 0.0;

	    for (l = 0; l < jx; ++l) {
	      lagr[0] += lagrx / (x - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagr[0] += lagrx / (x - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagr[1] += lagry / (y - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagr[1] += lagry / (y - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagr[2] += lagrz / (z - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagr[2] += lagrz / (z - pz->v[l]);
	    }

	    lagr[0] *= lagry * lagrz;
	    lagr[1] *= lagrx * lagrz;
	    lagr[2] *= lagrx * lagry;

	    sum += ww[q]
	      * (nt[0] * lagr[0] + nt[1] * lagr[1] + nt[2] * lagr[2]);

	  }

	  V->a[t + index * ld] = gt * sum;
	  index++;
	}
      }
    }
  }
}

void
assemble_bem3d_dn_lagrange_linear_amatrix(const uint * idx, pcavector px,
					  pcavector py, pcavector pz,
					  pcbem3d bem, pamatrix V)
{
  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const     real(*gr_n)[3] = (const real(*)[3]) gr->n;
  const preal gr_g = (const preal) gr->g;
  plistnode *v2t = bem->v2t;
  uint      rows = V->rows;
  field    *aa = V->a;
  uint      ld = V->ld;
  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single;
  real      base = bem->sq->base_single;
  real     *quad;
  uint      mx = px->dim;
  uint      my = py->dim;
  uint      mz = pz->dim;

  ptri_list tl, tl1;
  pvert_list vl;
  const real *A, *B, *C, *ns;
  real      lagr[3];
  uint      tri_sp[3];
  plistnode v;
  uint      s, ss, i, jx, jy, jz, l, k, q, cj, index;
  real      gs, sum, lagrx, lagry, lagrz, x, y, z, tx, sx, Ax, Bx, Cx;
  longindex ii, vv;

  quad = allocfield(bem->sq->n_single);

  clear_amatrix(V);

  tl = NULL;

  cj = 0;
  for (i = 0; i < rows; ++i) {
    ii = (idx == NULL ? i : idx[i]);
    for (v = v2t[ii], vv = v->data; v->next != NULL;
	 v = v->next, vv = v->data) {

      tl1 = tl;
      while (tl1 && tl1->t != vv) {
	tl1 = tl1->next;
      }

      if (tl1 == NULL) {
	tl1 = tl = new_tri_list(tl);
	tl->t = vv;
	cj++;
      }

      tl1->vl = new_vert_list(tl1->vl);
      tl1->vl->v = i;
    }
  }

  for (s = 0, tl1 = tl; s < cj; s++, tl1 = tl1->next) {
    ss = tl1->t;
    gs = gr_g[ss];
    ns = gr_n[ss];
    A = gr_x[gr_t[ss][0]];
    B = gr_x[gr_t[ss][1]];
    C = gr_x[gr_t[ss][2]];

    for (i = 0; i < 3; ++i) {
      tri_sp[i] = gr_t[ss][i];
    }

    index = 0;

    for (jx = 0; jx < mx; ++jx) {
      for (jy = 0; jy < my; ++jy) {
	for (jz = 0; jz < mz; ++jz) {

	  for (q = 0; q < nq; ++q) {
	    tx = xx[q];
	    sx = yy[q];
	    Ax = 1.0 - tx;
	    Bx = tx - sx;
	    Cx = sx;

	    x = A[0] * Ax + B[0] * Bx + C[0] * Cx;
	    y = A[1] * Ax + B[1] * Bx + C[1] * Cx;
	    z = A[2] * Ax + B[2] * Bx + C[2] * Cx;

	    lagrx = 1.0;
	    lagry = 1.0;
	    lagrz = 1.0;

	    /* TODO optimize lagrange polynomial eval */
	    for (l = 0; l < jx; ++l) {
	      lagrx *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagrx *= (x - px->v[l]) / (px->v[jx] - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagry *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagry *= (y - py->v[l]) / (py->v[jy] - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagrz *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagrz *= (z - pz->v[l]) / (pz->v[jz] - pz->v[l]);
	    }

	    lagr[0] = 0.0;
	    lagr[1] = 0.0;
	    lagr[2] = 0.0;

	    for (l = 0; l < jx; ++l) {
	      lagr[0] += lagrx / (x - px->v[l]);
	    }
	    for (l = jx + 1; l < mx; ++l) {
	      lagr[0] += lagrx / (x - px->v[l]);
	    }

	    for (l = 0; l < jy; ++l) {
	      lagr[1] += lagry / (y - py->v[l]);
	    }
	    for (l = jy + 1; l < my; ++l) {
	      lagr[1] += lagry / (y - py->v[l]);
	    }

	    for (l = 0; l < jz; ++l) {
	      lagr[2] += lagrz / (z - pz->v[l]);
	    }
	    for (l = jz + 1; l < mz; ++l) {
	      lagr[2] += lagrz / (z - pz->v[l]);
	    }

	    lagr[0] *= lagry * lagrz;
	    lagr[1] *= lagrx * lagrz;
	    lagr[2] *= lagrx * lagry;

	    quad[q] = ns[0] * lagr[0] + ns[1] * lagr[1] + ns[2] * lagr[2];
	  }

	  ww = bem->sq->w_single;
	  vl = tl1->vl;
	  while (vl) {
	    k = vl->v;
	    if (k < rows) {
	      ii = idx == NULL ? k : idx[k];
	      for (i = 0; i < 3; ++i) {
		if (ii == tri_sp[i]) {
		  sum = base;

		  for (q = 0; q < nq; ++q) {
		    sum += ww[q] * quad[q];
		  }

		  aa[k + index * ld] += sum * gs;
		}
		ww += nq;
	      }
	      ww = bem->sq->w_single;
	    }
	    vl = vl->next;
	  }

	  index++;
	}
      }
    }
  }

  del_tri_list(tl);
  freemem(quad);
}

void
projectl2_bem3d_const_avector(pbem3d bem, boundary_func3d rhs, pavector f)
{
  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const     real(*gr_n)[3] = (const real(*)[3]) gr->n;
  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single + 3 * nq;
  uint      n = f->dim;

  const real *A, *B, *C, *N;
  real      x[3];
  real      sum, tx, sx, Ax, Bx, Cx;
  uint      t, q;

  /*
   *  integrate function with constant basisfunctions
   */

  for (t = 0; t < n; ++t) {
    A = gr_x[gr_t[t][0]];
    B = gr_x[gr_t[t][1]];
    C = gr_x[gr_t[t][2]];
    N = gr_n[t];

    sum = 0.0;

    for (q = 0; q < nq; ++q) {
      tx = xx[q];
      sx = yy[q];
      Ax = 1.0 - tx;
      Bx = tx - sx;
      Cx = sx;

      x[0] = A[0] * Ax + B[0] * Bx + C[0] * Cx;
      x[1] = A[1] * Ax + B[1] * Bx + C[1] * Cx;
      x[2] = A[2] * Ax + B[2] * Bx + C[2] * Cx;

      sum += ww[q] * rhs(x, N);
    }

    f->v[t] = 2.0 * sum;
  }
}

static void
addeval_mass_linear_bem3d(field alpha, void *A, pcavector x, pavector y)
{
  pcbem3d   bem = (pcbem3d) A;
  pcsurface3d gr = bem->gr;
  plistnode *v2t = bem->v2t;
  preal     gr_g = gr->g;
  uint      n = x->dim;

  uint     *tri_k;
  plistnode v;
  uint      i, j;
  real      c1, c2;
  longindex vv;

  c1 = 1.0 / 24.0;
  c2 = 1.0 / 12.0;

  assert(y->dim == n);

  for (i = 0; i < n; ++i) {
    for (v = v2t[i], vv = v->data; v->next != NULL; v = v->next, vv = v->data) {
      tri_k = gr->t[vv];
      for (j = 0; j < 3; ++j) {
	if (i != tri_k[j]) {
	  y->v[i] += alpha * x->v[tri_k[j]] * gr_g[vv] * c1;
	}
      }
      y->v[i] += alpha * x->v[i] * gr_g[vv] * c2;
    }
  }
}

void
projectl2_bem3d_linear_avector(pbem3d bem, boundary_func3d rhs, pavector f)
{
  pcsurface3d gr = bem->gr;
  const     real(*gr_x)[3] = (const real(*)[3]) gr->x;
  const     uint(*gr_t)[3] = (const uint(*)[3]) gr->t;
  const     real(*gr_n)[3] = (const real(*)[3]) gr->n;
  const real *gr_g = (const real(*)) gr->g;
  const uint triangles = gr->triangles;
  const uint vertices = gr->vertices;
  uint      nq = bem->sq->n_single;
  real     *xx = bem->sq->x_single;
  real     *yy = bem->sq->y_single;
  real     *ww = bem->sq->w_single;
  real      base = bem->sq->base_single;

  pavector  v, r, p, a;
  const real *A, *B, *C, *N;
  field    *quad, sum;
  real      x[3];
  const uint *tri_t;
  real      tx, sx, Ax, Bx, Cx, gt_fac;
  uint      t, q, i;
  longindex ii;

  assert(vertices == f->dim);

  quad = allocfield(nq);
  v = new_avector(vertices);
  clear_avector(v);

  for (t = 0; t < triangles; t++) {
    tri_t = gr_t[t];
    gt_fac = gr_g[t];
    A = gr_x[tri_t[0]];
    B = gr_x[tri_t[1]];
    C = gr_x[tri_t[2]];
    N = gr_n[t];

    for (q = 0; q < nq; ++q) {
      tx = xx[q];
      sx = yy[q];
      Ax = 1.0 - tx;
      Bx = tx - sx;
      Cx = sx;

      x[0] = A[0] * Ax + B[0] * Bx + C[0] * Cx;
      x[1] = A[1] * Ax + B[1] * Bx + C[1] * Cx;
      x[2] = A[2] * Ax + B[2] * Bx + C[2] * Cx;

      quad[q] = rhs(x, N);
    }

    ww = bem->sq->w_single;

    for (i = 0; i < 3; ++i) {
      ii = tri_t[i];
      sum = base;

      for (q = 0; q < nq; ++q) {
	sum += ww[q] * quad[q];
      }

      assert(ii < vertices);
      v->v[ii] += sum * gt_fac;

      ww += nq;
    }
  }

  r = new_avector(vertices);
  p = new_avector(vertices);
  a = new_avector(vertices);
  random_avector(f);

  init_cg((addeval_t) addeval_mass_linear_bem3d, (void *) bem, v, f, r, p, a);

  while (norm2_avector(r) > 1.0e-15) {
    step_cg((addeval_t) addeval_mass_linear_bem3d, (void *) bem, v, f, r, p,
	    a);
  }

  del_avector(r);
  del_avector(p);
  del_avector(a);
  del_avector(v);
  freemem(quad);
}

prkmatrix
build_bem3d_rkmatrix(pccluster row, pccluster col, void *data)
{
  pcbem3d   bem = (pcbem3d) data;

  prkmatrix R;

  R = new_rkmatrix(row->size, col->size, 0);
  bem->farfield_rk(row, 0, col, 0, bem, R);

  return R;
}

pamatrix
build_bem3d_amatrix(pccluster row, pccluster col, void *data)
{
  pcbem3d   bem = (pcbem3d) data;

  pamatrix  N;

  N = new_amatrix(row->size, col->size);
  bem->nearfield(row->idx, col->idx, bem, false, N);

  return N;
}

/* ------------------------------------------------------------
 Initializerfunctions for h-matrix approximations
 ------------------------------------------------------------ */

void
setup_hmatrix_recomp_bem3d(pbem3d bem, bool recomp, real accur_recomp,
			   bool coarsen, real accur_coarsen)
{
  paprxbem3d aprx = bem->aprx;

  assert(accur_recomp >= 0.0);
  assert(accur_coarsen >= 0.0);

  uninit_recompression_bem3d(aprx);

  aprx->recomp = recomp;
  aprx->accur_recomp = accur_recomp;
  aprx->coarsen = coarsen;
  aprx->accur_coarsen = accur_coarsen;
}

/* ------------------------------------------------------------
 Interpolation
 ------------------------------------------------------------ */

void
setup_hmatrix_aprx_inter_row_bem3d(pbem3d bem, pccluster rc, pccluster cc,
				   pcblock tree, uint m)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->lagrange_row != NULL);

  setup_interpolation_bem3d(bem->aprx, m);

  bem->farfield_rk = assemble_bem3d_inter_row_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

void
setup_hmatrix_aprx_inter_col_bem3d(pbem3d bem, pccluster rc, pccluster cc,
				   pcblock tree, uint m)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->lagrange_col != NULL);

  setup_interpolation_bem3d(bem->aprx, m);

  bem->farfield_rk = assemble_bem3d_inter_col_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

void
setup_hmatrix_aprx_inter_mixed_bem3d(pbem3d bem, pccluster rc,
				     pccluster cc, pcblock tree, uint m)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->lagrange_row != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->lagrange_col != NULL);

  setup_interpolation_bem3d(bem->aprx, m);

  bem->farfield_rk = assemble_bem3d_inter_mixed_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

/* ------------------------------------------------------------
 Green
 ------------------------------------------------------------ */

void
setup_hmatrix_aprx_green_row_bem3d(pbem3d bem, pccluster rc, pccluster cc,
				   pcblock tree, uint m, uint l, real delta,
				   quadpoints3d quadpoints)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);

  bem->farfield_rk = assemble_bem3d_green_row_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

void
setup_hmatrix_aprx_green_col_bem3d(pbem3d bem, pccluster rc, pccluster cc,
				   pcblock tree, uint m, uint l, real delta,
				   quadpoints3d quadpoints)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);

  bem->farfield_rk = assemble_bem3d_green_col_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

void
setup_hmatrix_aprx_green_mixed_bem3d(pbem3d bem, pccluster rc,
				     pccluster cc, pcblock tree, uint m,
				     uint l, real delta,
				     quadpoints3d quadpoints)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);

  bem->farfield_rk = assemble_bem3d_green_mixed_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

/* ------------------------------------------------------------
 Greenhybrid
 ------------------------------------------------------------ */

void
setup_hmatrix_aprx_greenhybrid_row_bem3d(pbem3d bem, pccluster rc,
					 pccluster cc, pcblock tree, uint m,
					 uint l, real delta, real accur,
					 quadpoints3d quadpoints)
{
  pparbem3d par = bem->par;

  uint      n, i;

  (void) cc;
  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->nearfield != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_greenhybrid_row_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;

  n = par->grcnn;

  if (par->grcn != NULL && par->grcnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grcn[i] != NULL) {
	del_greencluster3d(par->grcn[i]);
      }
    }
    freemem(par->grcn);
  }

  n = rc->desc;

  par->grcn = (pgreencluster3d *) allocmem((size_t) n *
					   sizeof(pgreencluster3d));

  for (i = 0; i < n; ++i) {
    par->grcn[i] = NULL;
  }

  par->grcnn = n;
}

void
setup_hmatrix_aprx_greenhybrid_col_bem3d(pbem3d bem, pccluster rc,
					 pccluster cc, pcblock tree, uint m,
					 uint l, real delta, real accur,
					 quadpoints3d quadpoints)
{
  pparbem3d par = bem->par;

  uint      i, n;

  (void) rc;
  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);
  assert(bem->nearfield != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_greenhybrid_col_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;

  n = par->gccnn;

  if (par->gccn != NULL && par->gccnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gccn[i] != NULL) {
	del_greencluster3d(par->gccn[i]);
      }
    }
    freemem(par->gccn);
  }

  n = cc->desc;

  par->gccn = (pgreencluster3d *) allocmem((size_t) n *
					   sizeof(pgreencluster3d));

  for (i = 0; i < n; ++i) {
    par->gccn[i] = NULL;
  }

  par->gccnn = n;
}

void
setup_hmatrix_aprx_greenhybrid_mixed_bem3d(pbem3d bem, pccluster rc,
					   pccluster cc, pcblock tree, uint m,
					   uint l, real delta, real accur,
					   quadpoints3d quadpoints)
{
  pparbem3d par = bem->par;

  uint      i, n;

  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);
  assert(bem->nearfield != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_greenhybrid_mixed_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;

  n = par->grcnn;

  if (par->grcn != NULL && par->grcnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grcn[i] != NULL) {
	del_greencluster3d(par->grcn[i]);
      }
    }
    freemem(par->grcn);
  }

  n = rc->desc;

  par->grcn = (pgreencluster3d *) allocmem((size_t) n *
					   sizeof(pgreencluster3d));

  for (i = 0; i < n; ++i) {
    par->grcn[i] = NULL;
  }

  par->grcnn = n;

  n = par->gccnn;

  if (par->gccn != NULL && par->gccnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gccn[i] != NULL) {
	del_greencluster3d(par->gccn[i]);
      }
    }
    freemem(par->gccn);
  }

  n = cc->desc;

  par->gccn = (pgreencluster3d *) allocmem((size_t) n *
					   sizeof(pgreencluster3d));

  for (i = 0; i < n; ++i) {
    par->gccn[i] = NULL;
  }

  par->gccnn = n;
}

/* ------------------------------------------------------------
 ACA
 ------------------------------------------------------------ */

void
setup_hmatrix_aprx_aca_bem3d(pbem3d bem, pccluster rc, pccluster cc,
			     pcblock tree, real accur)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->nearfield != NULL);

  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_ACA_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

void
setup_hmatrix_aprx_paca_bem3d(pbem3d bem, pccluster rc, pccluster cc,
			      pcblock tree, real accur)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->nearfield != NULL);

  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_PACA_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

/* ------------------------------------------------------------
 HCA
 ------------------------------------------------------------ */

void
setup_hmatrix_aprx_hca_bem3d(pbem3d bem, pccluster rc, pccluster cc,
			     pcblock tree, uint m, real accur)
{

  (void) rc;
  (void) cc;
  (void) tree;

  assert(bem->kernels->fundamental != NULL);
  assert(bem->kernels->kernel_row != NULL);
  assert(bem->kernels->kernel_col != NULL);

  setup_interpolation_bem3d(bem->aprx, m);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = assemble_bem3d_HCA_rkmatrix;
  bem->farfield_u = NULL;

  bem->leaf_row = NULL;
  bem->leaf_col = NULL;
  bem->transfer_row = NULL;
  bem->transfer_col = NULL;
}

/* ------------------------------------------------------------
 Initializerfunctions for h2-matrix approximations
 ------------------------------------------------------------ */

void
setup_h2matrix_recomp_bem3d(pbem3d bem, bool hiercomp, real accur_hiercomp)
{
  paprxbem3d aprx = bem->aprx;

  assert(accur_hiercomp >= 0.0);

  aprx->hiercomp = hiercomp;
  aprx->accur_hiercomp = accur_hiercomp;
  aprx->tm = new_releucl_truncmode();
}

void
setup_h2matrix_aprx_inter_bem3d(pbem3d bem, pcclusterbasis rb,
				pcclusterbasis cb, pcblock tree, uint m)
{

  (void) rb;
  (void) cb;
  (void) tree;

  assert(bem->kernels->lagrange_row != NULL);
  assert(bem->kernels->lagrange_col != NULL);
  assert(bem->kernels->fundamental != NULL);

  setup_interpolation_bem3d(bem->aprx, m);

  bem->farfield_rk = NULL;
  bem->farfield_u = assemble_bem3d_inter_uniform;

  bem->leaf_row = assemble_bem3d_inter_row_clusterbasis;
  bem->leaf_col = assemble_bem3d_inter_col_clusterbasis;
  bem->transfer_row = assemble_bem3d_inter_transfer_clusterbasis;
  bem->transfer_col = assemble_bem3d_inter_transfer_clusterbasis;
}

void
setup_h2matrix_aprx_greenhybrid_bem3d(pbem3d bem, pcclusterbasis rb,
				      pcclusterbasis cb, pcblock tree, uint m,
				      uint l, real delta, real accur,
				      quadpoints3d quadpoints)
{
  pparbem3d par = bem->par;

  uint      i, n;

  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);
  assert(bem->nearfield != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = NULL;
  bem->farfield_u = assemble_bem3d_greenhybrid_uniform;

  bem->leaf_row = assemble_bem3d_greenhybrid_leaf_row_clusterbasis;
  bem->leaf_col = assemble_bem3d_greenhybrid_leaf_col_clusterbasis;
  bem->transfer_row = assemble_bem3d_greenhybrid_transfer_row_clusterbasis;
  bem->transfer_col = assemble_bem3d_greenhybrid_transfer_col_clusterbasis;

  n = par->grbnn;

  if (par->grbn != NULL && par->grbnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grbn[i] != NULL) {
	del_greenclusterbasis3d(par->grbn[i]);
      }
    }
    freemem(par->grbn);
  }

  n = rb->t->desc;

  par->grbn = (pgreenclusterbasis3d *) allocmem((size_t) n *
						sizeof(pgreenclusterbasis3d));

  for (i = 0; i < n; ++i) {
    par->grbn[i] = NULL;
  }

  par->grbnn = n;

  n = par->gcbnn;

  if (par->gcbn != NULL && par->gcbnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gcbn[i] != NULL) {
	del_greenclusterbasis3d(par->gcbn[i]);
      }
    }
    freemem(par->gcbn);
  }

  n = cb->t->desc;

  par->gcbn = (pgreenclusterbasis3d *) allocmem((size_t) n *
						sizeof(pgreenclusterbasis3d));

  for (i = 0; i < n; ++i) {
    par->gcbn[i] = NULL;
  }

  par->gcbnn = n;
}

void
setup_h2matrix_aprx_greenhybrid_ortho_bem3d(pbem3d bem, pcclusterbasis rb,
					    pcclusterbasis cb, pcblock tree,
					    uint m, uint l, real delta,
					    real accur,
					    quadpoints3d quadpoints)
{
  pparbem3d par = bem->par;

  uint      i, n;

  (void) tree;

  assert(quadpoints != NULL);
  assert(bem->kernels->fundamental_row != NULL);
  assert(bem->kernels->dnz_fundamental_row != NULL);
  assert(bem->kernels->kernel_col != NULL);
  assert(bem->kernels->dnz_kernel_col != NULL);
  assert(bem->nearfield != NULL);

  setup_green_bem3d(bem->aprx, m, l, delta, quadpoints);
  setup_aca_bem3d(bem->aprx, accur);

  bem->farfield_rk = NULL;
  bem->farfield_u = assemble_bem3d_greenhybridortho_uniform;

  bem->leaf_row = assemble_bem3d_greenhybridortho_leaf_row_clusterbasis;
  bem->leaf_col = assemble_bem3d_greenhybridortho_leaf_col_clusterbasis;
  bem->transfer_row =
    assemble_bem3d_greenhybridortho_transfer_row_clusterbasis;
  bem->transfer_col =
    assemble_bem3d_greenhybridortho_transfer_col_clusterbasis;
  n = par->grbnn;

  if (par->grbn != NULL && par->grbnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->grbn[i] != NULL) {
	del_greenclusterbasis3d(par->grbn[i]);
      }
    }
    freemem(par->grbn);
  }

  n = rb->t->desc;

  par->grbn = (pgreenclusterbasis3d *) allocmem((size_t) n *
						sizeof(pgreenclusterbasis3d));

  for (i = 0; i < n; ++i) {
    par->grbn[i] = NULL;
  }

  par->grbnn = n;

  n = par->gcbnn;

  if (par->gcbn != NULL && par->gcbnn != 0) {
    for (i = 0; i < n; ++i) {
      if (par->gcbn[i] != NULL) {
	del_greenclusterbasis3d(par->gcbn[i]);
      }
    }
    freemem(par->gcbn);
  }

  n = cb->t->desc;

  par->gcbn = (pgreenclusterbasis3d *) allocmem((size_t) n *
						sizeof(pgreenclusterbasis3d));

  for (i = 0; i < n; ++i) {
    par->gcbn[i] = NULL;
  }

  par->gcbnn = n;
}

/* ------------------------------------------------------------
 Fill hmatrix
 ------------------------------------------------------------ */

static void
assemble_bem3d_block_hmatrix(pcblock b, uint bname, uint rname,
			     uint cname, uint pardepth, void *data)
{
  pbem3d    bem = (pbem3d) data;
  paprxbem3d aprx = bem->aprx;
  pparbem3d par = bem->par;
  phmatrix *hn = par->hn;
  phmatrix  G = hn[bname];

  (void) b;
  (void) pardepth;

  if (G->r) {
    bem->farfield_rk(G->rc, rname, G->cc, cname, bem, G->r);
    if (aprx->recomp == true) {
      trunc_rkmatrix(0, aprx->accur_recomp, G->r);
    }
  }
  else if (G->f) {
    bem->nearfield(G->rc->idx, G->cc->idx, bem, false, G->f);
  }
}

static void
assemblecoarsen_bem3d_block_hmatrix(pcblock b, uint bname,
				    uint rname, uint cname, uint pardepth,
				    void *data)
{
  pbem3d    bem = (pbem3d) data;
  paprxbem3d aprx = bem->aprx;
  pparbem3d par = bem->par;
  phmatrix *hn = par->hn;
  phmatrix  G = hn[bname];

  (void) b;
  (void) pardepth;

  if (G->r) {
    bem->farfield_rk(G->rc, rname, G->cc, cname, bem, G->r);
    if (aprx->recomp == true) {
      trunc_rkmatrix(0, aprx->accur_recomp, G->r);
    }
  }
  else if (G->f) {
    bem->nearfield(G->rc->idx, G->cc->idx, bem, false, G->f);
  }
  else {
    assert(G->son != NULL);
    coarsen_hmatrix(G, aprx->accur_coarsen, false);
  }
}

void
assemble_bem3d_hmatrix(pbem3d bem, pblock b, phmatrix G)
{
  pparbem3d par = bem->par;
  par->hn = enumerate_hmatrix(b, G);

  iterate_byrow_block(b, 0, 0, 0, max_pardepth, NULL,
		      assemble_bem3d_block_hmatrix, bem);

  freemem(par->hn);
  par->hn = NULL;
}

void
assemblecoarsen_bem3d_hmatrix(pbem3d bem, pblock b, phmatrix G)
{
  pparbem3d par = bem->par;
  par->hn = enumerate_hmatrix(b, G);

  iterate_byrow_block(b, 0, 0, 0, max_pardepth, NULL,
		      assemblecoarsen_bem3d_block_hmatrix, bem);

  freemem(par->hn);
  par->hn = NULL;
}

/* ------------------------------------------------------------
 Fill h2-matrix
 ------------------------------------------------------------ */

static void
assemble_bem3d_block_h2matrix(pcblock b, uint bname, uint rname,
			      uint cname, uint pardepth, void *data)
{
  pbem3d    bem = (pbem3d) data;
  pparbem3d par = bem->par;
  ph2matrix *h2n = par->h2n;
  ph2matrix G = h2n[bname];

  (void) b;
  (void) pardepth;

  if (G->u) {
    bem->farfield_u(rname, cname, bem, G->u);
  }
  else if (G->f) {
    bem->nearfield(G->rb->t->idx, G->cb->t->idx, bem, false, G->f);
  }
}

static void
assemblehiercomp_bem3d_block_h2matrix(pcblock b, uint bname,
				      uint rname, uint cname, uint pardepth,
				      void *data)
{
  pbem3d    bem = (pbem3d) data;
  pparbem3d par = bem->par;
  paprxbem3d aprx = bem->aprx;
  ph2matrix *h2n = par->h2n;
  ph2matrix G = h2n[bname];
  pclusteroperator *rwn = par->rwn;
  pclusteroperator *cwn = par->cwn;
  uint     *leveln = par->leveln;
  uint      rsons = G->rsons;
  uint      csons = G->csons;
  pccluster rc = G->rb->t;
  pccluster cc = G->cb->t;
  uint      rows = rc->size;
  uint      cols = cc->size;
  ptruncmode tm = aprx->tm;
  real      eps = aprx->accur_hiercomp;

  prkmatrix R;
  pclusteroperator *rw1, *cw1;
  uint      bname1, i, j;

  (void) b;
  (void) pardepth;

  assert(rwn[bname] == 0);
  assert(cwn[bname] == 0);

  if (G->son) {
    rw1 = (pclusteroperator *) allocmem((size_t) sizeof(pclusteroperator) *
					rsons * csons);
    cw1 =
      (pclusteroperator *) allocmem((size_t) sizeof(pclusteroperator) *
				    rsons * csons);

    bname1 = bname + 1;

    for (j = 0; j < csons; j++) {
      for (i = 0; i < rsons; i++) {
	assert(rwn[bname1]);
	assert(cwn[bname1]);

	rw1[i + j * rsons] = rwn[bname1];
	cw1[i + j * rsons] = cwn[bname1];

	bname1 += b->son[i + j * rsons]->desc;
      }
    }

    assert(bname1 == bname + b->desc);

    /* Unify submatrices */
    unify_h2matrix(G, rw1, cw1, tm, eps * pow(tm->zeta_level, leveln[bname]),
		   rwn + bname, cwn + bname);

    /* Clean up */
    for (j = 0; j < csons; j++) {
      for (i = 0; i < rsons; i++) {
	del_clusteroperator(cw1[i + j * rsons]);
	del_clusteroperator(rw1[i + j * rsons]);
      }
    }
    freemem(cw1);
    freemem(rw1);
  }

  if (G->u) {
    R = new_rkmatrix(rows, cols, 0);

    bem->farfield_rk(rc, rname, cc, cname, bem, R);
    convert_rkmatrix_uniform(R, G->u, tm, rwn + bname, cwn + bname);

    ref_clusterbasis(&G->rb, G->u->rb);
    ref_clusterbasis(&G->cb, G->u->cb);

    del_rkmatrix(R);
  }
  else if (G->f) {
    bem->nearfield(rc->idx, cc->idx, bem, false, G->f);

    rwn[bname] = new_leaf_clusteroperator(rc);
    resize_clusteroperator(rwn[bname], 0, G->rb->k);
    cwn[bname] = new_leaf_clusteroperator(cc);
    resize_clusteroperator(cwn[bname], 0, G->cb->k);
  }
}

void
assemble_bem3d_h2matrix(pbem3d bem, pblock b, ph2matrix G)
{
  pparbem3d par = bem->par;
  par->h2n = enumerate_h2matrix(b, G);

  iterate_byrow_block(b, 0, 0, 0, max_pardepth, NULL,
		      assemble_bem3d_block_h2matrix, bem);

  freemem(par->h2n);
  par->h2n = NULL;
}

void
assemblehiercomp_bem3d_h2matrix(pbem3d bem, pblock b, ph2matrix G)
{
  pparbem3d par = bem->par;
  paprxbem3d aprx = bem->aprx;
  uint      blocks = b->desc;

  real      s;
  uint      i;

  par->h2n = enumerate_h2matrix(b, G);
  par->rwn = (pclusteroperator *) allocmem((size_t) sizeof(pclusteroperator) *
					   blocks);
  par->cwn =
    (pclusteroperator *) allocmem((size_t) sizeof(pclusteroperator) * blocks);
  par->leveln = enumerate_level_block(b);

  for (i = 0; i < blocks; ++i) {
    par->rwn[i] = NULL;
    par->cwn[i] = NULL;
  }

  s = REAL_POW(aprx->tm->zeta_level, getdepth_block(b));
  aprx->accur_hiercomp /= s;

  iterate_byrow_block(b, 0, 0, 0, max_pardepth, NULL,
		      assemblehiercomp_bem3d_block_h2matrix, bem);

  aprx->accur_hiercomp *= s;

  freemem(par->h2n);
  par->h2n = NULL;
  del_clusteroperator(par->rwn[0]);
  del_clusteroperator(par->cwn[0]);
  freemem(par->rwn);
  par->rwn = NULL;
  freemem(par->cwn);
  par->cwn = NULL;
  freemem(par->leveln);
  par->leveln = NULL;
}

static void
assemble_h2matrix_row_clusterbasis(pcclusterbasis rb, uint rname, void *data)
{
  pcbem3d   bem = (pcbem3d) data;

  if (rb->sons > 0) {
    assert(bem->transfer_row);
    bem->transfer_row(bem, (pclusterbasis) rb, rname);
  }
  else {
    assert(bem->leaf_row);
    bem->leaf_row(bem, (pclusterbasis) rb, rname);
  }
}

void
assemble_bem3d_h2matrix_row_clusterbasis(pcbem3d bem, pclusterbasis rb)
{
  iterate_parallel_clusterbasis((pcclusterbasis) rb, 0, max_pardepth, NULL,
				assemble_h2matrix_row_clusterbasis,
				(void *) bem);
}

static void
assemble_h2matrix_col_clusterbasis(pcclusterbasis cb, uint cname, void *data)
{
  pcbem3d   bem = (pcbem3d) data;

  if (cb->sons > 0) {
    assert(bem->transfer_col);
    bem->transfer_col(bem, (pclusterbasis) cb, cname);
  }
  else {
    assert(bem->leaf_col);
    bem->leaf_col(bem, (pclusterbasis) cb, cname);
  }
}

void
assemble_bem3d_h2matrix_col_clusterbasis(pcbem3d bem, pclusterbasis cb)
{
  iterate_parallel_clusterbasis((pcclusterbasis) cb, 0, max_pardepth, NULL,
				assemble_h2matrix_col_clusterbasis,
				(void *) bem);
}
