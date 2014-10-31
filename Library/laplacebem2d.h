/* ------------------------------------------------------------
 This is the file "laplacebem2d.h" of the H2Lib package.
 All rights reserved, Sven Christophersen 2011
 ------------------------------------------------------------ */

/**
 * @file laplacebem2d.h
 * @author Sven Christophersen
 * @date 2011
 */

#ifndef LAPLACEBEM2D_H_
#define LAPLACEBEM2D_H_

/* C STD LIBRARY */
/* CORE 0 */
#include "basic.h"
#include "parameters.h"
/* CORE 1 */
/* CORE 2 */
/* CORE 3 */
/* SIMPLE */
/* PARTICLES */
/* BEM */
#include "bem2d.h"

/** \defgroup laplacebem2d laplacebem2d
 *  @brief This module contains functions to setup and solve boundary integral
 *  equations for the Laplace operator in 2D.
 *  @{ */

/* ------------------------------------------------------------
 Constructors and destructors
 ------------------------------------------------------------ */

/**
 * @brief Creates a new @ref _bem2d "bem2d"-object for computation of
 * single layer potential matrix.
 *
 * After calling this function the resulting @ref _bem2d "bem"-object will
 * provide all functionality that is necessary to build up fully populated
 * single layer potential matrix @f$ V \in \mathbb R^{\mathcal I \times
 * \mathcal I}@f$ and also @ref _hmatrix "hmatrix"
 * or @ref _h2matrix "h2matrix" approximation of this matrix.
 *
 * @param gr Polygonal, two dimensional geometry.
 * @param q Order of gaussian quadrature used within computation of matrix entries.
 * @param basis Type of basis functions used for neumann data.
 *
 * @returns Returns a @ref _bem2d "bem"-object that can compute fully populated
 * slp matrices @f$ V @f$ for the laplace equation.
 */
HEADER_PREFIX pbem2d
new_slp_laplace_bem2d(pccurve2d gr, uint q, basisfunctionbem2d basis);

/**
 * @brief Creates a new @ref _bem2d "bem2d"-object for computation of
 * double layer potential matrix.
 *
 * After calling this function the resulting @ref _bem2d "bem"-object will
 * provide all functionality that is necessary to build up fully populated
 * double layer potential matrix @f$ K + \frac{1}{2} M \in \mathbb
 * R^{\mathcal I \times \mathcal J}@f$ and also @ref _hmatrix "hmatrix"
 * or @ref _h2matrix "h2matrix"
 * approximation of this matrix.
 *
 * @param gr Polygonal, two dimensional geometry.
 * @param q Order of gaussian quadrature used within computation of matrix entries.
 * @param basis_neumann Type of basis functions used for neumann data.
 * @param basis_dirichlet Type of basis functions used for dirichlet data.
 * @param alpha Double layer operator + @f$\alpha@f$ mass matrix.
 *
 * @returns Returns a @ref _bem2d "bem"-object that can compute fully populated
 * dlp matrices @f$ K + \frac{1}{2} M @f$ for the laplace equation.
 */
HEADER_PREFIX pbem2d
new_dlp_laplace_bem2d(pccurve2d gr, uint q, basisfunctionbem2d basis_neumann,
    basisfunctionbem2d basis_dirichlet, field alpha);

/* ------------------------------------------------------------
 Examples for Dirichlet- / Neumann-data to test linear system
 ------------------------------------------------------------ */

/**
 * @brief A simple linear harmonic function that will serve as dirichlet values.
 *
 * When computing the neumann data out of the dirichlet data one can use this
 * function as test data which will generate dirichlet values of with the following
 * values:
 * @f[
 * f(\vec x, \, \vec n) = x_1 + x_2
 * @f]
 * Corresponding neumann data can be generated by using
 * @ref eval_neumann_linear_laplacebem2d.
 * <br>
 * To build up an appropriate dirichlet data coefficient vector one needs the
 * @f$ L_2@f$-projection. This can be done by passing this function to
 * @ref projectl2_bem2d_const_avector for piecewise constant basis functions.
 *
 * @param x Evaluation point.
 * @param n Normal vector to current evaluation point.
 * @returns returns the function value of @f$ f(\vec x, \, \vec n) @f$.
 */
HEADER_PREFIX field
eval_dirichlet_linear_laplacebem2d(const real *x, const real *n);

/**
 * @brief A simple linear harmonic function that will serve as neumann values.
 *
 * When computing the neumann data out of the dirichlet data one can use this
 * function as test data which will generate neumann values of with the following
 * values:
 * @f[
 * f(\vec x, \, \vec n) = n_1 + n_2
 * @f]
 * Corresponding dirichlet data can be generated by using
 * @ref eval_dirichlet_linear_laplacebem2d.
 * <br>
 * To build up an appropriate neumann data coefficient vector one needs the
 * @f$ L_2@f$-projection. This can be done by passing this function to
 * @ref projectl2_bem2d_const_avector for piecewise constant basis functions.
 *
 * @param x Evaluation point.
 * @param n Normal vector to current evaluation point.
 * @returns returns the function value of @f$ f(\vec x, \, \vec n) @f$.
 */
HEADER_PREFIX field
eval_neumann_linear_laplacebem2d(const real *x, const real *n);

/**
 * @brief A simple quadratic harmonic function that will serve as dirichlet values.
 *
 * When computing the neumann data out of the dirichlet data one can use this
 * function as test data which will generate dirichlet values of with the following
 * values:
 * @f[
 * f(\vec x, \, \vec n) = x_1^2 - x_2^2
 * @f]
 * Corresponding neumann data can be generated by using
 * @ref eval_neumann_quadratic_laplacebem2d.
 * <br>
 * To build up an appropriate dirichlet data coefficient vector one needs the
 * @f$ L_2@f$-projection. This can be done by passing this function to
 * @ref projectl2_bem2d_const_avector for piecewise constant basis functions.
 *
 * @param x Evaluation point.
 * @param n Normal vector to current evaluation point.
 * @returns returns the function value of @f$ f(\vec x, \, \vec n) @f$.
 */
HEADER_PREFIX field
eval_dirichlet_quadratic_laplacebem2d(const real *x, const real *n);

/**
 * @brief A simple quadratic harmonic function that will serve as neumann values.
 *
 * When computing the neumann data out of the dirichlet data one can use this
 * function as test data which will generate neumann values of with the following
 * values:
 * @f[
 * f(\vec x, \, \vec n) = 2 \cdot x_1 \cdot n_1 - 2 \cdot x_2 \cdot n_2
 * @f]
 * Corresponding dirichlet data can be generated by using
 * @ref eval_dirichlet_quadratic_laplacebem2d.
 * <br>
 * To build up an appropriate neumann data coefficient vector one needs the
 * @f$ L_2@f$-projection. This can be done by passing this function to
 * @ref projectl2_bem2d_const_avector for piecewise constant basis functions.
 *
 * @param x Evaluation point.
 * @param n Normal vector to current evaluation point.
 * @returns returns the function value of @f$ f(\vec x, \, \vec n) @f$.
 */
HEADER_PREFIX field
eval_neumann_quadratic_laplacebem2d(const real *x, const real *n);

/**
 * @brief An interactive convenience function to create @ref _hmatrix "hmatrix"
 * or @ref _h2matrix "h2matrix"
 * approximations of boundary integral operators such as slp or dlp matrices.
 *
 * @param gr 2D polygonal geometry used for current problem.
 * @param op This char specifies the boundary integral operator to be constructed.
 * Valid values are 's' for single layer potential and 'd' for double layer
 * potential plus 0.5 times mass matrix.
 * @param basis_neumann Defines the basis functions used for neumann data.
 * @param basis_dirichlet Defines the basis functions used for dirichlet data.
 * @param q Order of gaussian quadrature used for nearfield entry computation.
 * @param G Pointer to the matrix that will be constructed after calling this
 * routine.
 * @param time Pointer to a real value, that will be filled with the elapsed time
 * for computing the desired matrix.
 * @param filename Name of a file where the parameters that are used to construct
 * the Matrix approximation will be stored in. If <tt>NULL</tt> is passed then
 * this parameter will be neglected.
 *
 * @returns Function will return type of the resulting matrix approximation.
 *
 * @attention Yet only 's' for single layer potential and 'd' for double layer
 * potential are admissible values for parameter <tt>op</tt>.
 */
HEADER_PREFIX matrixtype
build_interactive_laplacebem2d(pccurve2d gr, char op,
    basisfunctionbem2d basis_neumann, basisfunctionbem2d basis_dirichlet,
    uint q, void **G, real *time, char *filename);

/** @} */

#endif /* LAPLACEBEM2D_H_ */
