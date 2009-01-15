// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
/****************************************************************************/
/*  Header--File for extra Albert Functions                                 */
/****************************************************************************/
#ifndef DUNE_ALBERTAEXTRA_HH
#define DUNE_ALBERTAEXTRA_HH

#include <algorithm>
#include <cassert>
#include <cstring>

#include <dune/grid/albertagrid/albertaheader.hh>

#if HAVE_ALBERTA

#ifdef __ALBERTApp__
namespace Albert {
#endif

//*********************************************************************
//
//  Help Routines for the ALBERTA Mesh
//
//*********************************************************************
namespace AlbertHelp
{

  //*********************************************************************

  // Leaf Data for Albert, only the leaf elements have this data set
  template <int cdim, int vertices>
  struct AlbertLeafData
  {
    // type of stored data
    typedef struct
    {
      double determinant;
    } Data;

    // keep element numbers
    inline static void AlbertLeafRefine( EL *parent, EL *child[2] )
    {
      Data * ldata;
      int i;

      ldata = (Data *) parent->child[1];
      assert(ldata != 0);

      //std::cout << "Leaf refine for el = " << parent << "\n";

      double childDet = 0.5 * ldata->determinant;

      /* bisection ==> 2 children */
      for(i=0; i<2; i++)
      {
        Data *ldataChi = (Data *) child[i]->child[1];
        assert(ldataChi != 0);
        ldataChi->determinant = childDet;
      }
    }

    inline static void AlbertLeafCoarsen(EL *parent, EL *child[2])
    {
      Data *ldata;
      int i;

      ldata = (Data *) parent->child[1];
      assert(ldata != 0);
      double & det = ldata->determinant;
      det = 0.0;

      //std::cout << "Leaf coarsen for el = " << parent << "\n";

      /* bisection ==> 2 children */
      for(i=0; i<2; i++)
      {
        Data *ldataChi = (Data *) child[i]->child[1];
        assert(ldataChi != 0);
        det += ldataChi->determinant;
      }
    }

#if DUNE_ALBERTA_VERSION < 0x200
    // we dont need Leaf Data
    inline static void initLeafData(LEAF_DATA_INFO * linfo)
    {
      linfo->leaf_data_size = sizeof(Data);
      linfo->refine_leaf_data  = &AlbertLeafRefine;
      linfo->coarsen_leaf_data = &AlbertLeafCoarsen;
    }
#endif

    // function for mesh_traverse, is called on every element
    inline static void setLeafData(const EL_INFO * elf)
    {
      assert( elf->el->child[0] == 0 );
      Data *ldata = (Data *) elf->el->child[1];
      assert(ldata != 0);

      ldata->determinant = ALBERTA el_det(elf);
    }

    // remember on which level an element realy lives
    inline static void initLeafDataValues( MESH * mesh, int proc )
    {
      // see ALBERTA Doc page 72, traverse over all hierarchical elements
      ALBERTA meshTraverse(mesh,-1, CALL_LEAF_EL|FILL_COORDS,setLeafData);
    }

  }; // end of AlbertLeafData


#ifndef CALC_COORD
  // set entry for new elements to 1
  template <int dim>
  inline static void refineCoordsAndRefineCallBack ( DOF_REAL_D_VEC * drv , RC_LIST_EL *list, int ref)
  {
    //static MeshCallBack & callBack = MeshCallBack::instance();

    const int nv = drv->fe_space->admin->n0_dof[VERTEX];
    REAL_D* vec = 0;
    GET_DOF_VEC(vec,drv);
    assert(ref > 0);

    const EL * el = GET_EL_FROM_LIST(*list);

    // refinement edge is alwyas between vertex 0 and 1
    const int dof0 = el->dof[0][nv];
    const int dof1 = el->dof[1][nv];

    assert( el->child[0] );
    // new dof has local number dim
    const int dofnew = el->child[0]->dof[dim][nv];

    // get coordinates
    const REAL_D & oldCoordZero = vec[dof0];
    const REAL_D & oldCoordOne  = vec[dof1];
    REAL_D & newCoord = vec[dofnew];

    // new coordinate is average between old on same edge
    // see ALBERTA docu page 159, where this method is described
    // as real_refine_inter
    for(int j=0; j<dim; ++j)
      newCoord[j] = 0.5*(oldCoordZero[j] + oldCoordOne[j]);
  }
#endif

} // end namespace AlbertHelp

#ifdef __ALBERTApp__
} // end namespace Albert
#endif

#endif // HAVE_ALBERTA

#endif  /* !_ALBERTAEXTRA_H_ */
