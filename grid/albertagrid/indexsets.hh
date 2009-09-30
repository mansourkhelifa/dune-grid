// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_ALBERTAGRIDINDEXSETS_HH
#define DUNE_ALBERTAGRIDINDEXSETS_HH

#if HAVE_ALBERTA

#include <dune/common/stdstreams.hh>

#include <dune/grid/common/grid.hh>
#include <dune/grid/common/indexidset.hh>
#include <dune/grid/common/indexstack.hh>

#include <dune/grid/albertagrid/albertaheader.hh>
#include <dune/grid/albertagrid/misc.hh>
#include <dune/grid/albertagrid/dofadmin.hh>
#include <dune/grid/albertagrid/dofvector.hh>
#include <dune/grid/albertagrid/elementinfo.hh>

namespace Dune
{

  // External Forward Declarations
  // -----------------------------

  template< int dim, int dimworld >
  class AlbertaGrid;

  template< int codim, int dim, class GridImp >
  class AlbertaGridEntity;



  namespace Alberta
  {
    typedef Dune::IndexStack< int, 100000 > IndexStack;

    static IndexStack *currentIndexStack = 0;
  }



  // AlbertaGridHierarchicIndexSet
  // -----------------------------

  template< int dim, int dimworld >
  class AlbertaGridHierarchicIndexSet
    : public IndexSet< AlbertaGrid< dim, dimworld >, AlbertaGridHierarchicIndexSet< dim,dimworld >, int >
  {
    typedef AlbertaGridHierarchicIndexSet< dim, dimworld > This;
    typedef IndexSet< AlbertaGrid< dim, dimworld >, This, int > Base;

    friend class AlbertaGrid< dim, dimworld >;

  public:
    typedef AlbertaGrid< dim, dimworld > Grid;

    typedef typename Base::IndexType IndexType;

    static const int dimension = Grid::dimension;

    typedef Alberta::ElementInfo< dimension > ElementInfo;
    typedef Alberta::HierarchyDofNumbering< dimension > DofNumbering;

  private:
    typedef typename Grid::Traits Traits;

    typedef Alberta::DofVectorPointer< int > IndexVectorPointer;

    class InitEntityNumber;

    template< int codim >
    struct CreateEntityNumbers;

    template< int codim >
    class RefineNumbering;

    template< int codim >
    class CoarsenNumbering;

    explicit AlbertaGridHierarchicIndexSet ( const DofNumbering &dofNumbering );

  public:
    typedef Alberta::IndexStack IndexStack;

    //! return true if entity is contained in set
    template< class Entity >
    bool contains ( const Entity & ) const
    {
      return true;
    }

    //! return index of entity
    template< class Entity >
    IndexType index ( const Entity &entity ) const
    {
      const int codim = Entity::codimension;
      return index< codim >( entity );
    }

    //! return hierarchic index of given entity
    template< int codim >
    IndexType index ( const typename Grid::Traits::template Codim< codim >::Entity &entity ) const
    {
      const AlbertaGridEntity< codim, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );
      return subIndex( entityImp.elementInfo(), entityImp.subEntity(), codim );
    }

    //! import default implementation of subIndex<cc>
    //! \todo remove after next release
    using Base::subIndex;

    //! return subIndex of given enitiy's sub entity
    IndexType subIndex ( const typename Traits::template Codim< 0 >::Entity &entity, int i, unsigned int subcodim ) const
    {
      const AlbertaGridEntity< 0, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );
      const int j = entityImp.grid().generic2alberta( subcodim, i );
      return subIndex( entityImp.elementInfo(), j, subcodim );
    }

    //! return subIndex of given enitiy's sub entity
    template< int codim >
    IndexType subIndex ( const typename Traits::template Codim< codim >::Entity &entity, int i, unsigned int subcodim ) const
    {
      const AlbertaGridEntity< codim, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );

      const GenericReferenceElement< Alberta::Real, dimension > &refElement
        = GenericReferenceElements< Alberta::Real, dimension >::simplex();
      const int k = refElement.subEntity( entityImp.subEntity(), codim, i, codim+subcodim );

      const int j = entityImp.grid().generic2alberta( codim+subcodim, k );
      return subIndex( entityImp.elementInfo(), j, codim+subcodim );
    }

    //! return subIndex of given enitiy's sub entity
    template< class Entity >
    IndexType subIndex ( const Entity &entity, int i, unsigned int subcodim ) const
    {
      return subIndex< Entity::codimension >( entity, i, subcodim );
    }

    //! return size of set for given GeometryType
    IndexType size ( const GeometryType &type ) const
    {
      return (type.isSimplex() ? size( dimension - type.dim() ) : 0);
    }

    //! return size of set
    IndexType size ( int codim ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      return indexStack_[ codim ].size();
    }

    //! return geometry types this set has indices for
    const std::vector< GeometryType > &geomTypes( int codim ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      return geomTypes_[ codim ];
    }

    IndexType subIndex ( const ElementInfo &elementInfo, int i, unsigned int codim ) const
    {
      assert( !elementInfo == false );
      return subIndex( elementInfo.element(), i, codim );
    }

    /** \brief obtain hierarchic subindex
     *
     *  \param[in]  element  pointer to ALBERTA element
     *  \param[in]  i        number of the subelement (in ALBERTA numbering)
     *  \param[in]  codim    codimension
     */
    IndexType subIndex ( const Alberta::Element *element, int i, unsigned int codim ) const
    {
      int *array = (int *)entityNumbers_[ codim ];
      const int subIndex = array[ dofNumbering_( element, codim, i ) ];
      assert( (subIndex >= 0) && (subIndex < size( codim )) );
      return subIndex;
    }

    void preAdapt ()
    {
      // set global pointer to index stack
      if( !IndexVectorPointer::supportsAdaptationData )
      {
        assert( Alberta::currentIndexStack == 0 );
        Alberta::currentIndexStack = indexStack_;
      }
    }

    void postAdapt ()
    {
      // remove global pointer to index stack
      if( !IndexVectorPointer::supportsAdaptationData )
        Alberta::currentIndexStack = 0;
    }

    void create ()
    {
      Alberta::ForLoop< CreateEntityNumbers, 0, dimension >
      ::apply( dofNumbering_, *this );
    }

    void read ( const std::string &filename )
    {
      Alberta::ForLoop< CreateEntityNumbers, 0, dimension >
      ::apply( filename, dofNumbering_.mesh(), *this );
    }

    bool write ( const std::string &filename ) const
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

    void release ()
    {
      for( int i = 0; i <= dimension; ++i )
        entityNumbers_[ i ].release();
    }

  private:
    template< int codim >
    static IndexStack &getIndexStack ( const IndexVectorPointer &dofVector )
    {
      IndexStack *indexStack;
      if( IndexVectorPointer::supportsAdaptationData )
        indexStack = dofVector.getAdaptationData< IndexStack >();
      else
        indexStack = &Alberta::currentIndexStack[ codim ];
      assert( indexStack != 0 );
      return *indexStack;
    }

  private:
    // access to the dof vectors
    const DofNumbering &dofNumbering_;

    // index stacks providing new numbers during adaptation
    IndexStack indexStack_[ dimension+1 ];

    // dof vectors storing the (persistent) numbering
    IndexVectorPointer entityNumbers_[ dimension+1 ];

    // all geometry types contained in the grid
    std::vector< GeometryType > geomTypes_[ dimension+1 ];
  };



  template< int dim, int dimworld >
  inline AlbertaGridHierarchicIndexSet< dim, dimworld >
  ::AlbertaGridHierarchicIndexSet ( const DofNumbering &dofNumbering )
    : dofNumbering_( dofNumbering )
  {
    for( int codim = 0; codim <= dimension; ++codim )
    {
      const GeometryType type( GeometryType::simplex, dimension - codim );
      geomTypes_[ codim ].push_back( type );
    }
  }




  // AlbertaGridHierarchicIndexSet::InitEntityNumber
  // -----------------------------------------------

  template< int dim, int dimworld >
  class AlbertaGridHierarchicIndexSet< dim, dimworld >::InitEntityNumber
  {
    IndexStack &indexStack_;

  public:
    InitEntityNumber ( IndexStack &indexStack )
      : indexStack_( indexStack )
    {}

    void operator() ( int &dof )
    {
      dof = indexStack_.getIndex();
    }
  };



  // AlbertaGridHierarchicIndexSet::CreateEntityNumbers
  // --------------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  struct AlbertaGridHierarchicIndexSet< dim, dimworld >::CreateEntityNumbers
  {
    static void setup ( AlbertaGridHierarchicIndexSet< dim, dimworld > &indexSet )
    {
      IndexVectorPointer &entityNumbers = indexSet.entityNumbers_[ codim ];

      entityNumbers.template setupInterpolation< RefineNumbering< codim > >();
      entityNumbers.template setupRestriction< CoarsenNumbering< codim > >();
      entityNumbers.setAdaptationData( &(indexSet.indexStack_[ codim ]) );
    }

    static void apply ( const Alberta::HierarchyDofNumbering< dimension > &dofNumbering,
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

    static void apply ( const std::string &filename,
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
  };



  // AlbertaGridHierarchicIndexSet::RefineNumbering
  // ----------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  struct AlbertaGridHierarchicIndexSet< dim, dimworld >::RefineNumbering
  {
    static const int dimension = dim;
    static const int codimension = codim;

  private:
    typedef Alberta::Patch< dimension > Patch;
    typedef Alberta::DofAccess< dimension, codimension > DofAccess;

    IndexStack &indexStack_;
    IndexVectorPointer dofVector_;
    DofAccess dofAccess_;

    explicit RefineNumbering ( const IndexVectorPointer &dofVector )
      : indexStack_( getIndexStack< codimension >( dofVector ) ),
        dofVector_( dofVector ),
        dofAccess_( dofVector.dofSpace() )
    {}

  public:
    void operator() ( const Alberta::Element *child, int subEntity )
    {
      int *const array = (int *)dofVector_;
      const int dof = dofAccess_( child, subEntity );
      array[ dof ] = indexStack_.getIndex();
    }

    static void interpolateVector ( const IndexVectorPointer &dofVector,
                                    const Patch &patch )
    {
      RefineNumbering refineNumbering( dofVector );
      patch.forEachInteriorSubChild( refineNumbering );
    }
  };



  // AlbertaGridHierarchicIndexSet::CoarsenNumbering
  // -----------------------------------------------

  template< int dim, int dimworld >
  template< int codim >
  struct AlbertaGridHierarchicIndexSet< dim, dimworld >::CoarsenNumbering
  {
    static const int dimension = dim;
    static const int codimension = codim;

  private:
    typedef Alberta::Patch< dimension > Patch;
    typedef Alberta::DofAccess< dimension, codimension > DofAccess;

    IndexStack &indexStack_;
    IndexVectorPointer dofVector_;
    DofAccess dofAccess_;

    explicit CoarsenNumbering ( const IndexVectorPointer &dofVector )
      : indexStack_( getIndexStack< codimension >( dofVector ) ),
        dofVector_( dofVector ),
        dofAccess_( dofVector.dofSpace() )
    {}

  public:
    void operator() ( const Alberta::Element *child, int subEntity )
    {
      int *const array = (int *)dofVector_;
      const int dof = dofAccess_( child, subEntity );
      indexStack_.freeIndex( array[ dof ] );
    }

    static void restrictVector ( const IndexVectorPointer &dofVector,
                                 const Patch &patch )
    {
      CoarsenNumbering coarsenNumbering( dofVector );
      patch.forEachInteriorSubChild( coarsenNumbering );
    }
  };



  // AlbertaGridIndexSet
  // -------------------

  template< int dim, int dimworld >
  class AlbertaGridIndexSet
    : public IndexSet< AlbertaGrid< dim, dimworld >, AlbertaGridIndexSet< dim, dimworld >, int >
  {
    typedef AlbertaGridIndexSet< dim, dimworld > This;
    typedef IndexSet< AlbertaGrid< dim, dimworld >, This, int > Base;

  public:
    typedef AlbertaGrid< dim, dimworld > Grid;

    typedef typename Base::IndexType IndexType;

    static const int dimension = Grid::dimension;

    typedef Alberta::ElementInfo< dimension > ElementInfo;
    typedef Alberta::HierarchyDofNumbering< dimension > DofNumbering;

  private:
    typedef typename Grid::Traits Traits;

    template< int codim >
    struct Insert;

  public:
    explicit AlbertaGridIndexSet ( const DofNumbering &dofNumbering )
      : dofNumbering_( dofNumbering )
    {
      for( int codim = 0; codim <= dimension; ++codim )
      {
        indices_[ codim ] = 0;

        const GeometryType type( GeometryType::simplex, dimension - codim );
        geomTypes_[ codim ].push_back( type );
      }
    }

    ~AlbertaGridIndexSet ()
    {
      for( int codim = 0; codim <= dimension; ++codim )
        delete[] indices_[ codim ];
    }

    template< class Entity >
    bool contains ( const Entity &entity ) const
    {
      const int codim = Entity::codimension;

      const AlbertaGridEntity< codim, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );
      const Alberta::Element *element = entityImp.elementInfo().el();

      const IndexType *const array = indices_[ codim ];
      const IndexType subIndex = array[ dofNumbering_( element, codim, entityImp.subEntity() ) ];

      return (subIndex >= 0);
    }

    template< class Entity >
    IndexType index ( const Entity &entity ) const
    {
      const int codim = Entity::codimension;
      return index< codim >( entity );
    }

    template< int codim >
    IndexType index ( const typename Grid::Traits::template Codim< codim >::Entity &entity ) const
    {
      const AlbertaGridEntity< codim, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );
      return subIndex( entityImp.elementInfo(), entityImp.subEntity(), codim );
    }

    //! import default implementation of subIndex<cc>
    //! \todo remove after next release
    using Base::subIndex;

    //! return subIndex of given enitiy's sub entity
    IndexType subIndex ( const typename Traits::template Codim< 0 >::Entity &entity, int i, unsigned int subcodim ) const
    {
      const AlbertaGridEntity< 0, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );
      const int j = entityImp.grid().generic2alberta( subcodim, i );
      return subIndex( entityImp.elementInfo(), j, subcodim );
    }

    //! return subIndex of given enitiy's sub entity
    template< int codim >
    IndexType subIndex ( const typename Traits::template Codim< codim >::Entity &entity, int i, unsigned int subcodim ) const
    {
      const AlbertaGridEntity< codim, dim, const Grid > &entityImp
        = Grid::getRealImplementation( entity );

      const GenericReferenceElement< Alberta::Real, dimension > &refElement
        = GenericReferenceElements< Alberta::Real, dimension >::simplex();
      const int k = refElement.subEntity( entityImp.subEntity(), codim, i, codim+subcodim );

      const int j = entityImp.grid().generic2alberta( codim+subcodim, k );
      return subIndex( entityImp.elementInfo(), j, codim+subcodim );
    }

    //! return subIndex of given enitiy's sub entity
    template< class Entity >
    IndexType subIndex ( const Entity &entity, int i, unsigned int subcodim ) const
    {
      return subIndex< Entity::codimension >( entity, i, subcodim );
    }

    IndexType size ( const GeometryType &type ) const
    {
      return (type.isSimplex() ? size( dimension - type.dim() ) : 0);
    }

    IndexType size ( int codim ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      return size_[ codim ];
    }

    const std::vector< GeometryType > &geomTypes( int codim ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      return geomTypes_[ codim ];
    }

    template< class Iterator >
    void update ( const Iterator &begin, const Iterator &end )
    {
      for( int codim = 0; codim <= dimension; ++codim )
      {
        delete[] indices_[ codim ];

        const unsigned int dofSize = dofNumbering_.size( codim );
        indices_[ codim ] = new IndexType[ dofSize ];
        for( unsigned int i = 0; i < dofSize; ++i )
          indices_[ codim ][ i ] = -1;

        size_[ codim ] = 0;
      }

      for( Iterator it = begin; it != end; ++it )
      {
        const AlbertaGridEntity< 0, dim, const Grid > &entityImp
          = Grid::getRealImplementation( *it );
        const Alberta::Element *element = entityImp.elementInfo().el();
        Alberta::ForLoop< Insert, 0, dimension >::apply( element, *this );
      }
    }

  private:
    IndexType subIndex ( const ElementInfo &elementInfo, int i, unsigned int codim ) const
    {
      assert( !elementInfo == false );
      return subIndex( elementInfo.element(), i, codim );
    }

    /** \brief obtain subindex
     *
     *  \param[in]  element  pointer to ALBERTA element
     *  \param[in]  i        number of the subelement (in ALBERTA numbering)
     *  \param[in]  codim    codimension
     */
    IndexType subIndex ( const Alberta::Element *element, int i, unsigned int codim ) const
    {
      const IndexType *const array = indices_[ codim ];
      const IndexType subIndex = array[ dofNumbering_( element, codim, i ) ];
      assert( (subIndex >= 0) && (subIndex < size( codim )) );
      return subIndex;
    }

    // access to the dof vectors
    const DofNumbering &dofNumbering_;

    // an array of indices for each codimension
    IndexType *indices_[ dimension+1 ];

    // the size of each codimension
    IndexType size_[ dimension+1 ];

    // all geometry types contained in the grid
    std::vector< GeometryType > geomTypes_[ dimension+1 ];
  };



  // AlbertaGridIndexSet::Insert
  // ---------------------------

  template< int dim, int dimworld >
  template< int codim >
  struct AlbertaGridIndexSet< dim, dimworld >::Insert
  {
    static void apply ( const Alberta::Element *const element,
                        AlbertaGridIndexSet< dim, dimworld > &indexSet )
    {
      int *const array = indexSet.indices_[ codim ];
      IndexType &size = indexSet.size_[ codim ];

      for( int i = 0; i < Alberta::NumSubEntities< dim, codim >::value; ++i )
      {
        int &index = array[ indexSet.dofNumbering_( element, codim, i ) ];
        if( index < 0 )
          index = size++;
      }
    }
  };



  // AlbertaGridIdSet
  // ----------------

  //! hierarchic index set of AlbertaGrid
  template< int dim, int dimworld >
  class AlbertaGridIdSet
    : public IdSet
      < AlbertaGrid< dim, dimworld >, AlbertaGridIdSet< dim, dimworld >, unsigned int >
  {
    typedef AlbertaGridIdSet< dim, dimworld > This;
    typedef AlbertaGrid< dim, dimworld > Grid;
    typedef IdSetDefaultImplementation< Grid, This, unsigned int > Base;

    friend class AlbertaGrid< dim, dimworld >;

    static const int dimension = Grid::dimension;

    typedef typename Grid::HierarchicIndexSet HierarchicIndexSet;

    const HierarchicIndexSet &hIndexSet_;

    //! create id set, only allowed for AlbertaGrid
    AlbertaGridIdSet ( const HierarchicIndexSet &hIndexSet )
      : hIndexSet_( hIndexSet )
    {}

  public:
    //! export type of id
    typedef typename Base::IdType IdType;

    /** \copydoc IdSet::id(const EntityType &e) const */
    template< class Entity >
    IdType id ( const Entity &e ) const
    {
      const int codim = Entity::codimension;
      return id< codim >( e );
    }

    /** \copydoc IdSet::id(const typename remove_const<GridImp>::type::Traits::template Codim<cc>::Entity &e) const */
    template< int codim >
    IdType id ( const typename Grid::template Codim< codim >::Entity &e ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      const IdType index = hIndexSet_.index( e );
      return (index << 2) | IdType( codim );
    }

    /** \copydoc IdSet::subId(const typename remove_const<GridImp>::type::Traits::template Codim<0>::Entity &e,int i,unsigned int codim) const */
    IdType subId ( const typename Grid::template Codim< 0 >::Entity &e,
                   int i, int codim ) const
    {
      assert( (codim >= 0) && (codim <= dimension) );
      const IdType index = hIndexSet_.subIndex( e, i, codim );
      return (index << 2) | IdType( codim );
    }
  };

} // namespace Dune

#endif // #if HAVE_ALBERTA

#endif // #ifndef DUNE_ALBERTAGRIDINDEXSETS_HH
