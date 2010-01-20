// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include <config.h>

#if HAVE_ALBERTA

#include <dune/grid/albertagrid/indexsets.hh>

// can we get rid of this?
#include <dune/grid/albertagrid/agrid.hh>

namespace Dune
{

  namespace Alberta
  {
    IndexStack *currentIndexStack = 0;
  }



  // Implementation of AlbertaGridHierarchicIndexSet::CreateEntityNumbers
  // --------------------------------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  inline void
  AlbertaGridHierarchicIndexSet< dim, dimworld >::CreateEntityNumbers< codim >
  ::setup ( AlbertaGridHierarchicIndexSet< dim, dimworld > &indexSet )
  {
    IndexVectorPointer &entityNumbers = indexSet.entityNumbers_[ codim ];

    entityNumbers.template setupInterpolation< RefineNumbering< codim > >();
    entityNumbers.template setupRestriction< CoarsenNumbering< codim > >();
    entityNumbers.setAdaptationData( &(indexSet.indexStack_[ codim ]) );
  }


  template< int dim, int dimworld >
  template< int codim >
  inline void
  AlbertaGridHierarchicIndexSet< dim, dimworld >::CreateEntityNumbers< codim >
  ::apply ( const Alberta::HierarchyDofNumbering< dimension > &dofNumbering,
            AlbertaGridHierarchicIndexSet< dim, dimworld > &indexSet )
  {
    const Alberta::DofSpace *dofSpace = dofNumbering.dofSpace( codim );

    std::ostringstream s;
    s << "Numbering for codimension " << codim;
    indexSet.entityNumbers_[ codim ].create( dofSpace, s.str() );

    InitEntityNumber init( indexSet.indexStack_[ codim ] );
    indexSet.entityNumbers_[ codim ].forEach( init );

    setup( indexSet );
  }


  template< int dim, int dimworld >
  template< int codim >
  inline void
  AlbertaGridHierarchicIndexSet< dim, dimworld >::CreateEntityNumbers< codim >
  ::apply ( const std::string &filename,
            const Alberta::MeshPointer< dimension > &mesh,
            AlbertaGridHierarchicIndexSet< dim, dimworld > &indexSet )
  {
    std::ostringstream s;
    s << filename << ".cd" << codim;
    indexSet.entityNumbers_[ codim ].read( s.str(), mesh );

    const int maxIndex = max( indexSet.entityNumbers_[ codim ] );
    indexSet.indexStack_[ codim ].setMaxIndex( maxIndex + 1 );

    setup( indexSet );
  }



  // Implementation of AlbertaGridHierarchicIndexSet::RefineNumbering
  // ----------------------------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::RefineNumbering< codim >
  ::operator() ( const Alberta::Element *child, int subEntity )
  {
    int *const array = (int *)dofVector_;
    const int dof = dofAccess_( child, subEntity );
    array[ dof ] = indexStack_.getIndex();
  }


  template< int dim, int dimworld >
  template< int codim >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::RefineNumbering< codim >
  ::interpolateVector ( const IndexVectorPointer &dofVector, const Patch &patch )
  {
    RefineNumbering refineNumbering( dofVector );
    patch.forEachInteriorSubChild( refineNumbering );
  }



  // Implementation of AlbertaGridHierarchicIndexSet::CoarsenNumbering
  // -----------------------------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::CoarsenNumbering< codim >
  ::operator() ( const Alberta::Element *child, int subEntity )
  {
    int *const array = (int *)dofVector_;
    const int dof = dofAccess_( child, subEntity );
    indexStack_.freeIndex( array[ dof ] );
  }


  template< int dim, int dimworld >
  template< int codim >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::CoarsenNumbering< codim >
  ::restrictVector ( const IndexVectorPointer &dofVector, const Patch &patch )
  {
    CoarsenNumbering coarsenNumbering( dofVector );
    patch.forEachInteriorSubChild( coarsenNumbering );
  }



  // Implementation of AlbertaGridHierarchicIndexSet
  // -----------------------------------------------

  template< int dim, int dimworld >
  AlbertaGridHierarchicIndexSet< dim, dimworld >
  ::AlbertaGridHierarchicIndexSet ( const DofNumbering &dofNumbering )
    : dofNumbering_( dofNumbering )
  {
    for( int codim = 0; codim <= dimension; ++codim )
    {
      const GeometryType type( GeometryType::simplex, dimension - codim );
      geomTypes_[ codim ].push_back( type );
    }
  }


  template< int dim, int dimworld >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::create ()
  {
    ForLoop< CreateEntityNumbers, 0, dimension >::apply( dofNumbering_, *this );
  }


  template< int dim, int dimworld >
  void AlbertaGridHierarchicIndexSet< dim, dimworld >::read ( const std::string &filename )
  {
    ForLoop< CreateEntityNumbers, 0, dimension >::apply( filename, dofNumbering_.mesh(), *this );
  }


  template< int dim, int dimworld >
  bool AlbertaGridHierarchicIndexSet< dim, dimworld >::write ( const std::string &filename ) const
  {
    bool success = true;
    for( int i = 0; i <= dimension; ++i )
    {
      std::ostringstream s;
      s << filename << ".cd" << i;
      success &= entityNumbers_[ i ].write( s.str() );
    }
    return success;
  }



  // Instantiation
  // -------------

  template struct AlbertaGridHierarchicIndexSet< 1, Alberta::dimWorld >;
#if ALBERTA_DIM >= 2
  template struct AlbertaGridHierarchicIndexSet< 2, Alberta::dimWorld >;
#endif // #if ALBERTA_DIM >= 2
#if ALBERTA_DIM >= 3
  template struct AlbertaGridHierarchicIndexSet< 3, Alberta::dimWorld >;
#endif // #if ALBERTA_DIM >= 3

}

#endif // #if HAVE_ALBERTA
