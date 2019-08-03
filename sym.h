/**
 * A key component of the two-phase algorithm is the reduction of possible cube states by symmetries. The idea of this
 * is that "similar" cubes, i.e. ones that can be transformed into each other by shuffling around the faces and
 * performing some recoloring, are also "similar" to solve. Utilizing these symmetries allows to reduce the size of
 * pruning by roughly a factor of 16 (or 4 when using only 5 faces).
 *
 * All 48 different cube-symmetries can be generated by a composition of 4 basic ones:
 *  - Mirroring through a plane between the L and the R faces (2 options)
 *  - 90 degree rotation around an axis going through the U and the D faces (4 options)
 *  - 180 degree rotation around an axis going through the F and the B faces (2 options)
 *  - 120 degree rotation around an axis going through the URF and the DLB corner (3 options)
 *
 * While there are 48 symmetries, we can unfortunately not use all of them for symmetry reduction, since some do not
 * preserve the UD-slice, which is however critical for phase 2. Therefore we can only apply the first 16 symmetries
 * (all that do not involve any 120 degree rotation). In the 5-faces variant, a rotation through the FB-axis creates
 * similar problems as it moves the B-face meaning that we can only reduce by 4 symmetries.
 *
 * Similar to moves, symmetries can also be represented by `CubieCube`s. However, we need to introduce additional
 * corner orientations to properly handle multiplication with mirrored cubes. This solver uses values >= 3 to indicate
 * a mirrored state. Note also that these special orientations will only every occur as a temporary result during a
 * symmetry transform.
 *
 * Efficiently carrying out symmetry reduction requires several lookup tables. First, we need a table `*_sym` that
 * maps a raw-coordinate `c` to its symmetry reduced counterpart `sc`. Since we always also want to know the symmetry
 * `s` used to perform this transformation, we store in this table not just `sc` but directly `N_SYMS_SUB * sc + s` to
 * not only save a bit of space but also to avoid another slow table-lookup. Next, we need to store a representative
 * raw-coordinate for every symmetry-coordinate so that we can generate the pruning tables, this happens in the arrays
 * `*_raw`. Finally, not every symmetry class contains exactly `N_SYMS_SUB` cube states (hence the `N_SYMS_SUB` memory
 * reduction is only approximate) but there are some so called "self-symmetries", i.e. symmetries that map the class
 * representative to itself. The `*_selfs` tables collect those as they are also important for generating correct
 * pruning tables.
 *
 * At last, to considerably decrease table size, we do not perform symmetry reduction on a combined coordinate
 * (consisting of say `c1` and `c2`), but rather only symmetry reduce `c1` (using the tables mentioned in the previous
 * paragraph) and then conjugate `c2` before merging them together again. Therefore we also need `conj_*` tables.
 *
 * The actual mechanics of a symmetry transformation are discussed in "sym.cc". See also "prun.h" for a description of
 * why we define the tables here only for certain coordinates.
 */

#ifndef SYM_H_
#define SYM_H_

#include <stdint.h>
#include "coord.h"
#include "cubie.h"

#define N_SYMS 48 // total number of symmetries

#ifdef FACES5
  #define N_SYMS_SUB 4 // number of symmetries used for reduction
  #define N_FSLICE_SYM 255664 // number of FSLICE sym-coordinates
  #define N_CPERM_SYM 10368 // number of CPERM sym-coordinates
  #define ROT_SYM 36 // symmetry used for multi-threading in the search (90 degree rotation around F-B axis)
#else
  #define N_SYMS_SUB 16
  #define N_FSLICE_SYM 64430
  #define N_CPERM_SYM 2768
  #define ROT_SYM 16
#endif

/* Macros for easily handling the sym-coordinates joined with their respective symmetry */
#define SYMCOORD(coord, sym) (N_SYMS_SUB * coord + sym)
#define SYM(scoord) (scoord % N_SYMS_SUB)
#define COORD(scoord) (scoord / N_SYMS_SUB)

/* `CubieCube`s representing the basic symmetries */
const CubieCube kLR2Cube = {
  {UFL, URF, UBR, ULB, DLF, DFR, DRB, DBL},
  {UL, UF, UR, UB, DL, DF, DR, DB, FL, FR, BR, BL},
  {3, 3, 3, 3, 3, 3, 3, 3}, {} // note the special mirror corner orientation
};
const CubieCube kU4Cube = {
  {UBR, URF, UFL, ULB, DRB, DFR, DLF, DBL},
  {UB, UR, UF, UL, DB, DR, DF, DL, BR, FR, FL, BL},
  {}, {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}
};
const CubieCube kF2Cube = {
  {DLF, DFR, DRB, DBL, UFL, URF, UBR, ULB},
  {DL, DF, DR, DB, UL, UF, UR, UB, FL, FR, BR, BL},
  {}, {}
};
const CubieCube kURF3Cube = {
  {URF, DFR, DLF, UFL, UBR, DRB, DBL, ULB},
  {UF, FR, DF, FL, UB, BR, DB, BL, UR, DR, DL, UL},
  {1, 2, 1, 2, 2, 1, 2, 1},
  {1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1}
};

// `CubieCube`s representing all symmetries
extern CubieCube sym_cubes[N_SYMS];
// Invert a symmetry
extern int inv_sym[N_SYMS];
// Conjugate a move
extern int conj_move[N_MOVES][N_SYMS];

/* Coordinate conjugation tables */
extern uint16_t (*conj_twist)[N_SYMS_SUB];
extern uint16_t (*conj_udedges)[N_SYMS_SUB];

/* Tables used for the symmetry reduction */
extern uint32_t *fslice_sym;
extern uint32_t *cperm_sym;
extern uint32_t *fslice_raw;
extern uint16_t *cperm_raw;
extern uint16_t *fslice_selfs;
extern uint16_t *cperm_selfs;

// Sets up all the basic data; to be called before accessing anything from this file
void initSym();
// Generates all tables for the symmetry reduction (computationally expensive)
void initSymTables();

#endif
