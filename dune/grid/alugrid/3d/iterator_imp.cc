// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include "alu3dinclude.hh"

//#if COMPILE_ALUGRID_INLINE_FILES

#include "geometry.hh"
#include "entity.hh"
#include "grid.hh"
#include "faceutility.hh"

#include <dune/common/misc.hh>

namespace Dune {

  /************************************************************************************
  ###
  #     #    #   #####  ######  #####    ####   ######   ####      #     #####
  #     ##   #     #    #       #    #  #       #       #    #     #       #
  #     # #  #     #    #####   #    #   ####   #####   #          #       #
  #     #  # #     #    #       #####        #  #       #          #       #
  #     #   ##     #    #       #   #   #    #  #       #    #     #       #
  ###    #    #     #    ######  #    #   ####   ######   ####      #       #
  ************************************************************************************/

  // --IntersectionIterator
  template<class GridImp>
  inline ALU3dGridIntersectionIterator<GridImp> ::
  ALU3dGridIntersectionIterator(const GridImp & grid,
                                int wLevel) :
    geoProvider_(connector_),
    grid_(grid),
    item_(0),
    ghost_(0),
    index_(0),
    intersectionGlobal_(GeometryImp()),
    intersectionGlobalImp_(grid_.getRealImplementation(intersectionGlobal_)),
    intersectionSelfLocal_(GeometryImp()),
    intersectionSelfLocalImp_(grid_.getRealImplementation(intersectionSelfLocal_)),
    intersectionNeighborLocal_(GeometryImp()),
    intersectionNeighborLocalImp_(grid_.getRealImplementation(intersectionNeighborLocal_)),
    done_(true)
  {}

  // --IntersectionIterator
  template<class GridImp>
  inline ALU3dGridIntersectionIterator<GridImp> ::
  ALU3dGridIntersectionIterator(const GridImp & grid,
                                HElementType *el,
                                int wLevel,bool end) :
    connector_(),
    geoProvider_(connector_),
    grid_(grid),
    item_(0),
    ghost_(0),
    index_(0),
    intersectionGlobal_(GeometryImp()),
    intersectionGlobalImp_(grid_.getRealImplementation(intersectionGlobal_)),
    intersectionSelfLocal_(GeometryImp()),
    intersectionSelfLocalImp_(grid_.getRealImplementation(intersectionSelfLocal_)),
    intersectionNeighborLocal_(GeometryImp()),
    intersectionNeighborLocalImp_(grid_.getRealImplementation(intersectionNeighborLocal_)),
    done_(end)
  {
    if (!end)
    {
      setFirstItem(*el,wLevel);
    }
    else
    {
      done();
    }
  }

  template<class GridImp>
  inline void
  ALU3dGridIntersectionIterator<GridImp> :: done ()
  {
    done_  = true;
    item_  = 0;
#if ALU3DGRID_PARALLEL
    ghost_ = 0;
#endif
  }

  template<class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp> ::
  setFirstItem (const HElementType & elem, int wLevel)
  {
    ghost_      = 0;
    item_       = static_cast<const IMPLElementType *> (&elem);

    // Get first face
    const GEOFaceType* firstFace = getFace(*item_, index_);

    const GEOFaceType* childFace = firstFace->down();
    if( childFace ) firstFace = childFace;

    // Store the face in the connector
    setNewFace(*firstFace);
  }

  template<class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp> ::
  setInteriorItem (const HElementType & elem, const PLLBndFaceType& ghost, int wLevel)
  {
    // get correct face number
    index_ = ElementTopo::alu2duneFace( ghost.getGhost().second );

    // store ghost for method inside
    ghost_   = &ghost;

    // Get first face
    const GEOFaceType* firstFace = getFace( ghost, index_ );
    item_   = static_cast<const IMPLElementType *> (&elem);

    const GEOFaceType* childFace = firstFace->down();
    if( childFace ) firstFace = childFace;

    // Store the face in the connector
    setGhostFace(*firstFace);
  }

  template<class GridImp>
  template <class EntityType>
  inline void ALU3dGridIntersectionIterator<GridImp> ::
  first (const EntityType & en, int wLevel)
  {
    if( ! en.isLeaf() )
    {
      done();
      return ;
    }

    done_   = false;
    innerLevel_ = en.level();
    index_  = 0;

#if ALU3DGRID_PARALLEL
    if( en.isGhost() )
    {
      setInteriorItem(en.getItem(), en.getGhost(), wLevel);
    }
    else
#endif
    {
      assert( numFaces == en.getItem().nFaces() );
      setFirstItem(en.getItem(), wLevel);
    }
  }

  // copy constructor
  template<class GridImp>
  inline ALU3dGridIntersectionIterator<GridImp> ::
  ALU3dGridIntersectionIterator(const ALU3dGridIntersectionIterator<GridImp> & org) :
    connector_(org.connector_),
    geoProvider_(connector_),
    grid_(org.grid_),
    item_(org.item_),
    ghost_(org.ghost_),
    intersectionGlobal_(GeometryImp()),
    intersectionGlobalImp_(grid_.getRealImplementation(intersectionGlobal_)),
    intersectionSelfLocal_(GeometryImp()),
    intersectionSelfLocalImp_(grid_.getRealImplementation(intersectionSelfLocal_)),
    intersectionNeighborLocal_(GeometryImp()),
    intersectionNeighborLocalImp_(grid_.getRealImplementation(intersectionNeighborLocal_)),
    done_(org.done_)
  {
    if(org.item_)
    { // else it's a end iterator
      item_        = org.item_;
      innerLevel_  = org.innerLevel_;
      index_       = org.index_;
    }
    else
    {
      done();
    }
  }

  // copy constructor
  template<class GridImp>
  inline void
  ALU3dGridIntersectionIterator<GridImp> ::
  assign(const ALU3dGridIntersectionIterator<GridImp> & org)
  {
    if(org.item_)
    {
      // else it's a end iterator
      item_       = org.item_;
      ghost_      = org.ghost_;
      innerLevel_ = org.innerLevel_;
      index_      = org.index_;
      connector_.updateFaceInfo(org.connector_.face(),innerLevel_,
                                item_->twist(ElementTopo::dune2aluFace(index_)));
      geoProvider_.resetFaceGeom();
    }
    else {
      done();
    }
  }

  // check whether entities are the same or whether iterator is done
  template<class GridImp>
  inline bool ALU3dGridIntersectionIterator<GridImp> ::
  equals (const ALU3dGridIntersectionIterator<GridImp> & i ) const
  {
    // this method is only to check equality of real iterators and end
    // iterators
    return ((item_ == i.item_) &&
            (done_ == i.done_)
            );
  }

  template<class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp> :: increment ()
  {
    // leaf increment
    assert(item_);

    const GEOFaceType * nextFace = 0;

    // When neighbour element is refined, try to get the next child on the face
    if (connector_.conformanceState() == FaceInfoType::REFINED_OUTER) {
      nextFace = connector_.face().next();

      // There was a next child face...
      if (nextFace)
      {
#ifdef ALU3DGRID_PARALLEL
        if( ghost_ )
        {
          setGhostFace( *nextFace );
        }
        else
#endif
        {
          setNewFace(*nextFace);
        }
        return; // we found what we were looking for...
      }
    } // end if

    // Next face number of starting element
    ++index_;

    // When the face number is larger than the number of faces an element
    // can have, we've reached the end...
    // for ghost elements here is finito
    if (index_ >= numFaces || ghost_ )
    {
      this->done();
      return;
    }

    // ... else we can take the next face
    nextFace = getFace(connector_.innerEntity(), index_);
    assert(nextFace);

    // Check whether we need to go down first
    //if (nextFace has children which need to be visited)
    const GEOFaceType * childFace = nextFace->down();
    if( childFace ) nextFace = childFace;

    assert(nextFace);
    setNewFace(*nextFace);
    return;
  }


  template<class GridImp>
  inline typename ALU3dGridIntersectionIterator<GridImp>::EntityPointer
  ALU3dGridIntersectionIterator<GridImp>::outside () const
  {
    assert( this->neighbor() );
    // make sure that outside is not called for an end iterator
#if ALU3DGRID_PARALLEL
    if(connector_.ghostBoundary())
    {
      // create entity pointer with ghost boundary face
      return EntityPointer(this->grid_, connector_.boundaryFace() );
    }
#endif
    assert( &connector_.outerEntity() );
    return EntityPointer(this->grid_, connector_.outerEntity() );
  }

  template<class GridImp>
  inline typename ALU3dGridIntersectionIterator<GridImp>::EntityPointer
  ALU3dGridIntersectionIterator<GridImp>::inside () const
  {
#if ALU3DGRID_PARALLEL
    if( ghost_ )
    {
      return EntityPointer(this->grid_, *ghost_ );
    }
    else
#endif
    {
      // make sure that inside is not called for an end iterator
      return EntityPointer(this->grid_, connector_.innerEntity() );
    }
  }

  template<class GridImp>
  inline bool ALU3dGridIntersectionIterator<GridImp> :: boundary () const
  {
    return (connector_.outerBoundary());
  }

  template<class GridImp>
  inline bool ALU3dGridIntersectionIterator<GridImp>::neighbor () const
  {
    return ! boundary();
  }

  template<class GridImp>
  inline bool ALU3dGridIntersectionIterator<GridImp>::levelNeighbor () const
  {
    return false;
  }

  template<class GridImp>
  inline bool ALU3dGridIntersectionIterator<GridImp>::leafNeighbor () const
  {
    return neighbor();
  }


  template<class GridImp>
  inline int
  ALU3dGridIntersectionIterator< GridImp >::indexInInside () const
  {
    assert(ElementTopo::dune2aluFace(index_) == connector_.innerALUFaceIndex());
    return index_;
  }

  template <class GridImp>
  inline const typename ALU3dGridIntersectionIterator<GridImp>::LocalGeometry &
  ALU3dGridIntersectionIterator< GridImp >::geometryInInside () const
  {
    buildLocalGeometries();
    return intersectionSelfLocal_;
  }


  template< class GridImp >
  inline int
  ALU3dGridIntersectionIterator< GridImp >::indexInOutside () const
  {
    return ElementTopo::alu2duneFace( connector_.outerALUFaceIndex() );
  }

  template<>
  inline int ALU3dGridIntersectionIterator< const ALU3dGrid<3,3,hexa> >::twistInInside () const
  {
    const int aluTwist = connector_.innerTwist();
    const int mappedZero =
      FaceTopo::twist(ElementTopo::dune2aluFaceVertex( indexInInside(), 0), aluTwist);

    return
      (ElementTopo::faceOrientation( indexInInside() ) * sign(aluTwist) < 0 ?
       mappedZero : -mappedZero-1);
  }

  template<>
  inline int ALU3dGridIntersectionIterator< const ALU3dGrid<3 ,3, tetra> >::twistInInside () const
  {
    return connector_.duneTwist( indexInInside(), connector_.innerTwist() );
  }

  template<>
  inline int ALU3dGridIntersectionIterator< const ALU3dGrid<3,3,hexa> >::twistInOutside () const
  {
    const int aluTwist = connector_.outerTwist();
    const int mappedZero =
      FaceTopo::twist(ElementTopo::dune2aluFaceVertex( indexInOutside(), 0), aluTwist);

    return
      (ElementTopo::faceOrientation( indexInOutside() ) * sign(aluTwist) < 0 ?
       mappedZero : -mappedZero-1);
  }

  template<>
  inline int ALU3dGridIntersectionIterator< const ALU3dGrid<3 ,3, tetra> >::twistInOutside () const
  {
    return connector_.duneTwist( indexInOutside(), connector_.outerTwist() );
  }

  template <class GridImp>
  inline const typename ALU3dGridIntersectionIterator<GridImp>::LocalGeometry &
  ALU3dGridIntersectionIterator< GridImp >::geometryInOutside () const
  {
    assert(neighbor());
    buildLocalGeometries();
    return intersectionNeighborLocal_;
  }

  template<class GridImp>
  inline typename ALU3dGridIntersectionIterator<GridImp>::NormalType &
  ALU3dGridIntersectionIterator<GridImp>::
  integrationOuterNormal(const FieldVector<alu3d_ctype, dim-1>& local) const
  {
    return this->outerNormal(local);
  }

  template<class GridImp>
  inline typename ALU3dGridIntersectionIterator<GridImp>::NormalType &
  ALU3dGridIntersectionIterator<GridImp>::
  outerNormal(const FieldVector<alu3d_ctype, dim-1>& local) const
  {
    assert(item_ != 0);
    return geoProvider_.outerNormal(local);
  }

  template<class GridImp>
  inline typename ALU3dGridIntersectionIterator<GridImp>::NormalType &
  ALU3dGridIntersectionIterator<GridImp>::
  unitOuterNormal(const FieldVector<alu3d_ctype, dim-1>& local) const
  {
    unitOuterNormal_ = this->outerNormal(local);
    unitOuterNormal_ *= (1.0/unitOuterNormal_.two_norm());
    return unitOuterNormal_;
  }

  template<class GridImp>
  inline const typename ALU3dGridIntersectionIterator<GridImp>::Geometry &
  ALU3dGridIntersectionIterator< GridImp >::geometry () const
  {
    geoProvider_.buildGlobalGeom(intersectionGlobalImp_);
    return intersectionGlobal_;
  }


  template<>
  inline GeometryType
  ALU3dGridIntersectionIterator< const ALU3dGrid< 3, 3, tetra > >::type () const
  {
    return GeometryType( GeometryType::simplex, dim-1 );
  }

  template<>
  inline GeometryType
  ALU3dGridIntersectionIterator< const ALU3dGrid< 3, 3, hexa > >::type () const
  {
    return GeometryType( GeometryType::cube, dim-1 );
  }


  template<class GridImp>
  inline int
  ALU3dGridIntersectionIterator<GridImp>::boundaryId () const
  {
    assert(item_);
    return (boundary() ? connector_.boundaryFace().bndtype() : 0);
  }

  template<class GridImp>
  inline size_t
  ALU3dGridIntersectionIterator<GridImp>::boundarySegmentIndex() const
  {
    assert(item_);
#ifdef ALUGRID_VERTEX_PROJECTION
    assert( boundary() );
    return connector_.boundaryFace().segmentIndex();
#else
    derr << "Method available in any version of ALUGrid > 1.14 \n";
    return 0;
#endif
  }

  template <class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp>::buildLocalGeometries() const
  {
    intersectionSelfLocalImp_.buildGeom(geoProvider_.intersectionSelfLocal());
    if (!connector_.outerBoundary()) {
      intersectionNeighborLocalImp_.buildGeom(geoProvider_.intersectionNeighborLocal());
    }
  }

  template <class GridImp>
  inline const ALU3dImplTraits<tetra>::GEOFaceType*
  ALU3dGridIntersectionIterator<GridImp>::
  getFace(const GEOTriangleBndType & bnd, int index) const
  {
    return bnd.myhface3(0);
  }

  template <class GridImp>
  inline const ALU3dImplTraits<hexa>::GEOFaceType*
  ALU3dGridIntersectionIterator<GridImp>::
  getFace(const GEOQuadBndType & bnd, int index) const
  {
    return bnd.myhface4(0);
  }

  template <class GridImp>
  inline const ALU3dImplTraits<tetra>::GEOFaceType*
  ALU3dGridIntersectionIterator<GridImp>::
  getFace(const GEOTetraElementType & elem, int index) const {
    assert(index >= 0 && index < numFaces);
    return elem.myhface3(ElementTopo::dune2aluFace(index));
  }

  template <class GridImp>
  inline const ALU3dImplTraits<hexa>::GEOFaceType*
  ALU3dGridIntersectionIterator<GridImp>::
  getFace(const GEOHexaElementType & elem, int index) const {
    assert(index >= 0 && index < numFaces);
    return elem.myhface4(ElementTopo::dune2aluFace(index));
  }

  template <class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp>::
  setNewFace(const GEOFaceType& newFace)
  {
    assert( ! ghost_ );
    assert( innerLevel_ == item_->level() );
    connector_.updateFaceInfo(newFace,innerLevel_,
                              item_->twist(ElementTopo::dune2aluFace(index_)) );
    geoProvider_.resetFaceGeom();
  }

  template <class GridImp>
  inline void ALU3dGridIntersectionIterator<GridImp>::
  setGhostFace(const GEOFaceType& newFace)
  {
    assert( ghost_ );
    assert( innerLevel_ == ghost_->level() );
    connector_.updateFaceInfo(newFace,innerLevel_, ghost_->twist(0) );
    geoProvider_.resetFaceGeom();
  }

  template <class GridImp>
  inline int
  ALU3dGridIntersectionIterator<GridImp>::
  level() const {
    assert( item_ );
    return item_->level();
  }

  /************************************************************************************
  ###
  #     #    #   #####  ######  #####    ####   ######   ####      #     #####
  #     ##   #     #    #       #    #  #       #       #    #     #       #
  #     # #  #     #    #####   #    #   ####   #####   #          #       #
  #     #  # #     #    #       #####        #  #       #          #       #
  #     #   ##     #    #       #   #   #    #  #       #    #     #       #
  ###    #    #     #    ######  #    #   ####   ######   ####      #       #
  ************************************************************************************/

  // --IntersectionIterator
  template<class GridImp>
  inline ALU3dGridLevelIntersectionIterator<GridImp> ::
  ALU3dGridLevelIntersectionIterator(const GridImp & grid,
                                     int wLevel)
    : ALU3dGridIntersectionIterator<GridImp>(grid,wLevel)
      , levelNeighbor_(false)
      , isLeafItem_(false)
  {}

  // --IntersectionIterator
  template<class GridImp>
  inline ALU3dGridLevelIntersectionIterator<GridImp> ::
  ALU3dGridLevelIntersectionIterator(const GridImp & grid,
                                     HElementType *el,
                                     int wLevel,bool end)
    : ALU3dGridIntersectionIterator<GridImp>(grid,el,wLevel,end)
      , levelNeighbor_(false)
      , isLeafItem_(false)
  {}

  template<class GridImp>
  template <class EntityType>
  inline void ALU3dGridLevelIntersectionIterator<GridImp> ::
  first (const EntityType & en, int wLevel)
  {
    // if given Entity is not leaf, we create an end iterator
    this->done_   = false;
    this->index_  = 0;
    isLeafItem_   = en.isLeaf();

#if ALU3DGRID_PARALLEL
    if( en.isGhost() )
    {
      setInteriorItem(en.getItem(), en.getGhost(), wLevel);
    }
    else
#endif
    {
      assert( numFaces == en.getItem().nFaces() );
      setFirstItem(en.getItem(), wLevel);
    }
  }

  template<class GridImp>
  inline void ALU3dGridLevelIntersectionIterator<GridImp> ::
  setFirstItem (const HElementType & elem, int wLevel)
  {
    this->ghost_       = 0;
    this->item_        = static_cast<const IMPLElementType *> (&elem);
    this->innerLevel_  = wLevel;
    // Get first face
    const GEOFaceType* firstFace = getFace(*this->item_, this->index_);
    // Store the face in the connector
    setNewFace(*firstFace);
  }

  template<class GridImp>
  inline void ALU3dGridLevelIntersectionIterator<GridImp> ::
  setInteriorItem (const HElementType & elem, const PLLBndFaceType& ghost, int wLevel)
  {
    // store ghost for method inside
    this->ghost_   = &ghost;
    this->item_   = static_cast<const IMPLElementType *> (&elem);
    // get correct face number
    this->index_ = ElementTopo::alu2duneFace( ghost.getGhost().second );

    this->innerLevel_  = wLevel;

    // Get first face
    const GEOFaceType* firstFace = this->getFace( ghost, this->index_ );

    // Store the face in the connector
    setNewFace(*firstFace);
  }

  // copy constructor
  template<class GridImp>
  inline ALU3dGridLevelIntersectionIterator<GridImp> ::
  ALU3dGridLevelIntersectionIterator(const ThisType & org)
    : ALU3dGridIntersectionIterator<GridImp>(org)
      , levelNeighbor_(org.levelNeighbor_)
      , isLeafItem_(org.isLeafItem_)
  {}

  // copy constructor
  template<class GridImp>
  inline void
  ALU3dGridLevelIntersectionIterator<GridImp> ::
  assign(const ALU3dGridLevelIntersectionIterator<GridImp> & org)
  {
    ALU3dGridIntersectionIterator<GridImp>::assign(org);
    levelNeighbor_ = org.levelNeighbor_;
    isLeafItem_    = org.isLeafItem_;
  }

  template<class GridImp>
  inline void ALU3dGridLevelIntersectionIterator<GridImp> :: increment ()
  {
    // level increment
    assert(this->item_);

    // Next face number of starting element
    ++this->index_;

    // When the face number is larger than the number of faces an element
    // can have, we've reached the end...
    if (this->index_ >= numFaces || this->ghost_ ) {
      this->done();
      return;
    }

    // ... else we can take the next face
    const GEOFaceType * nextFace = this->getFace(this->connector_.innerEntity(), this->index_);
    assert(nextFace);

    setNewFace(*nextFace);
    return;
  }

  template<class GridImp>
  inline bool ALU3dGridLevelIntersectionIterator<GridImp>::neighbor () const
  {
    return levelNeighbor();
  }

  template<class GridImp>
  inline bool ALU3dGridLevelIntersectionIterator<GridImp>::levelNeighbor () const
  {
    return levelNeighbor_ && (! this->boundary());
  }

  template<class GridImp>
  inline bool ALU3dGridLevelIntersectionIterator<GridImp>::leafNeighbor () const
  {
    return false;
  }


  template <class GridImp>
  inline void ALU3dGridLevelIntersectionIterator<GridImp>::
  setNewFace(const GEOFaceType& newFace)
  {
    assert( this->item_->level() == this->innerLevel_ );
    levelNeighbor_ = (newFace.level() == this->innerLevel_);
    this->connector_.updateFaceInfo(newFace,this->innerLevel_,
#if ALU3DGRID_PARALLEL
                                    (this->ghost_) ? this->ghost_->twist(0) :
#endif
                                    this->item_->twist(ElementTopo::dune2aluFace(this->index_)));
    this->geoProvider_.resetFaceGeom();

    // check again level neighbor because outer element might be coarser then
    // this element
    if( isLeafItem_ )
    {
      if( this->connector_.ghostBoundary() )
      {
        const BNDFaceType & ghost = this->connector_.boundaryFace();
        // if nonconformity occurs then no level neighbor
        levelNeighbor_ = (this->innerLevel_ == ghost.ghostLevel() );
      }
      else if ( ! this->connector_.outerBoundary() )
      {
        levelNeighbor_ = (this->connector_.outerEntity().level() == this->innerLevel_);
      }
    }
  }
  /*************************************************************************
  #       ######  #    #  ######  #          #     #####  ######  #####
  #       #       #    #  #       #          #       #    #       #    #
  #       #####   #    #  #####   #          #       #    #####   #    #
  #       #       #    #  #       #          #       #    #       #####
  #       #        #  #   #       #          #       #    #       #   #
  ######  ######    ##    ######  ######     #       #    ######  #    #
  *************************************************************************/
  //--LevelIterator
  // Constructor for begin iterator
  template<int codim, PartitionIteratorType pitype, class GridImp >
  inline ALU3dGridLevelIterator<codim,pitype,GridImp> ::
  ALU3dGridLevelIterator(const GridImp & grid, int level, bool )
    : ALU3dGridEntityPointer<codim,GridImp> (grid,level)
      , level_(level)
      , iter_ (0)
  {
    iter_  = new IteratorType ( this->grid_ , level_, grid.nlinks() );
    assert( iter_ );
    this->firstItem(this->grid_,*this,level_);
  }

  // Constructor for end iterator
  template<int codim, PartitionIteratorType pitype, class GridImp >
  inline ALU3dGridLevelIterator<codim,pitype,GridImp> ::
  ALU3dGridLevelIterator(const GridImp & grid, int level)
    : ALU3dGridEntityPointer<codim,GridImp> (grid ,level)
      , level_(level)
      , iter_ (0)
  {
    this->done();
  }

  template<int codim, PartitionIteratorType pitype, class GridImp >
  inline ALU3dGridLevelIterator<codim,pitype,GridImp> ::
  ALU3dGridLevelIterator(const ALU3dGridLevelIterator<codim,pitype,GridImp> & org )
    : ALU3dGridEntityPointer<codim,GridImp> ( org.grid_ , org.level_ )
      , level_( org.level_ )
      , iter_(0)
  {
    assign(org);
  }

  template<int codim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLevelIterator<codim, pitype, GridImp> ::
  ~ALU3dGridLevelIterator ()
  {
    removeIter();
  }

  template<int codim, PartitionIteratorType pitype, class GridImp>
  inline void ALU3dGridLevelIterator<codim, pitype, GridImp> ::
  removeIter ()
  {
    this->done();
    if(iter_)
    {
      delete iter_;
      iter_ = 0;
    }
  }

  template<int codim, PartitionIteratorType pitype, class GridImp>
  inline void ALU3dGridLevelIterator<codim, pitype, GridImp> ::
  assign(const ThisType & org)
  {
    assert( iter_ == 0 );
    ALU3dGridEntityPointer <codim,GridImp> :: clone (org);
    level_ = org.level_;
    if( org.iter_ )
    {
      iter_ = new IteratorType ( *(org.iter_) );
      assert( iter_ );
      if(!(iter_->done()))
      {
        this->setItem(this->grid_, *this, *iter_, level_ );
        assert( this->equals(org) );
      }
    }
    else
    {
      this->done();
    }
  }
  template<int codim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLevelIterator<codim, pitype, GridImp> &
  ALU3dGridLevelIterator<codim, pitype, GridImp> ::
  operator = (const ThisType & org)
  {
    removeIter();
    assign(org);
    return *this;
  }

  template<int codim, PartitionIteratorType pitype, class GridImp >
  inline void ALU3dGridLevelIterator<codim,pitype,GridImp> :: increment ()
  {
    this->incrementIterator(this->grid_,*this,level_);
    return ;
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline typename ALU3dGridLevelIterator<cdim, pitype, GridImp> :: Entity &
  ALU3dGridLevelIterator<cdim, pitype, GridImp> :: dereference () const
  {
#ifndef NDEBUG
    const ALU3dGridLevelIterator<cdim, pitype, GridImp> endIterator (this->grid_,level_);
    // assert that iterator not equals end iterator
    assert( ! this->equals(endIterator) );
#endif

    // don't dereference empty entity pointer
    assert( this->item_ );
    assert( this->entity_ );
    assert( this->item_ == & this->entityImp().getItem() );
    return (*this->entity_);
  }

  //*******************************************************************
  //
  //  LEAFITERATOR
  //
  //--LeafIterator
  //*******************************************************************
  // constructor for end iterators
  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  ALU3dGridLeafIterator( const GridImp &grid, int level )
    : ALU3dGridEntityPointer <cdim,GridImp> ( grid, level )
      , iter_ (0)
      , walkLevel_(level)
  {
    this->done();
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  ALU3dGridLeafIterator(const GridImp &grid, int level ,
                        bool isBegin)
    : ALU3dGridEntityPointer <cdim,GridImp> ( grid, level )
      , iter_ (0)
      , walkLevel_(level)
  {
    // create interior iterator
    iter_ = new IteratorType ( this->grid_ , level , grid.nlinks() );
    assert( iter_ );
    // -1 to identify as leaf iterator
    this->firstItem(this->grid_,*this,-1);
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  ALU3dGridLeafIterator(const ThisType & org)
    : ALU3dGridEntityPointer <cdim,GridImp> ( org.grid_, org.walkLevel_ )
      , iter_(0)
      , walkLevel_(org.walkLevel_)
  {
    // assign iterator without cloning entity pointer again
    assign(org);
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  ~ALU3dGridLeafIterator()
  {
    removeIter();
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline void ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  removeIter ()
  {
    this->done();
    if(iter_)
    {
      delete iter_;
      iter_ = 0;
    }
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline ALU3dGridLeafIterator<cdim, pitype, GridImp> &
  ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  operator = (const ThisType & org)
  {
    removeIter();
    assign(org);
    return *this;
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline void ALU3dGridLeafIterator<cdim, pitype, GridImp> ::
  assign (const ThisType & org)
  {
    assert( iter_ == 0 );
    ALU3dGridEntityPointer <cdim,GridImp> :: clone (org);

    if( org.iter_ )
    {
      assert( !org.iter_->done() );
      iter_ = new IteratorType ( *(org.iter_) );
      assert( iter_ );

      if( !(iter_->done() ))
      {
        assert( !iter_->done());
        assert( !org.iter_->done() );
        // -1 to identify leaf iterator
        this->setItem(this->grid_,*this, *iter_,-1);
        assert( this->equals(org) );
      }
    }
    else
    {
      this->done();
    }

    walkLevel_ = org.walkLevel_;
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline void ALU3dGridLeafIterator<cdim, pitype, GridImp> :: increment ()
  {
    // -1 to identify leaf iterator
    this->incrementIterator(this->grid_,*this,-1);
    return ;
  }

  template<int cdim, PartitionIteratorType pitype, class GridImp>
  inline typename ALU3dGridLeafIterator<cdim, pitype, GridImp> :: Entity &
  ALU3dGridLeafIterator<cdim, pitype, GridImp> :: dereference () const
  {
#ifndef NDEBUG
    const ALU3dGridLeafIterator<cdim, pitype, GridImp> endIterator (this->grid_, this->grid_.maxLevel());
    // assert that iterator not equals end iterator
    assert( ! this->equals(endIterator) );
#endif

    // don't dereference empty entity pointer
    assert( this->item_ );
    assert( this->entity_ );
    assert( this->item_ == & this->entityImp().getItem() );
    return (*this->entity_);
  }


  /************************************************************************************
  #     #
  #     #     #    ######  #####      #     #####  ######  #####
  #     #     #    #       #    #     #       #    #       #    #
  #######     #    #####   #    #     #       #    #####   #    #
  #     #     #    #       #####      #       #    #       #####
  #     #     #    #       #   #      #       #    #       #   #
  #     #     #    ######  #    #     #       #    ######  #    #
  ************************************************************************************/
  // --HierarchicIterator
  template <class GridImp>
  inline ALU3dGridHierarchicIterator<GridImp> ::
  ALU3dGridHierarchicIterator(const GridImp & grid ,
                              const HElementType & elem, int maxlevel ,bool end)
    : ALU3dGridEntityPointer<0,GridImp> ( grid, maxlevel )
      , elem_(&elem)
      , ghost_( 0 )
      , nextGhost_( 0 )
      , maxlevel_(maxlevel)
  {
    if (!end)
    {
      HElementType * item =
        const_cast<HElementType *> (elem.down());
      if(item)
      {
        // we have children and they lie in the disired level range
        if(item->level() <= maxlevel_)
        {
          this->updateEntityPointer( item );
        }
        else
        { // otherwise do nothing
          this->done();
        }
      }
      else
      {
        this->done();
      }
    }
  }

#ifdef ALU3DGRID_PARALLEL
  template <class GridImp>
  inline ALU3dGridHierarchicIterator<GridImp> ::
  ALU3dGridHierarchicIterator(const GridImp & grid ,
                              const HBndSegType& ghost,
                              int maxlevel,
                              bool end)
    : ALU3dGridEntityPointer<0,GridImp> ( grid, maxlevel )
      , elem_( 0 )
      , ghost_( &ghost )
      , nextGhost_( 0 )
      , maxlevel_(maxlevel)
  {
    if( ! end )
    {
      // lock entity pointer
      this->locked_ = true ;
      nextGhost_ = const_cast<HBndSegType *> (ghost.down());

      // we have children and they lie in the disired level range
      if( nextGhost_ && nextGhost_->ghostLevel() <= maxlevel_)
      {
        this->updateGhostPointer( *nextGhost_ );
      }
      else
      { // otherwise do nothing
        this->done();
      }
    }
    else
    {
      this->done();
    }
  }
#endif

  template <class GridImp>
  inline ALU3dGridHierarchicIterator<GridImp> ::
  ALU3dGridHierarchicIterator(const ThisType& org)
    : ALU3dGridEntityPointer<0,GridImp> ( org.grid_, org.maxlevel_ )
  {
    assign( org );
  }

  template <class GridImp>
  inline ALU3dGridHierarchicIterator<GridImp> &
  ALU3dGridHierarchicIterator<GridImp> ::
  operator = (const ThisType& org)
  {
    assign( org );
    return *this;
  }

  template <class GridImp>
  inline void
  ALU3dGridHierarchicIterator<GridImp> ::
  assign(const ThisType& org)
  {
    // copy my data
    elem_      = org.elem_;
#ifdef ALU3DGRID_PARALLEL
    ghost_     = org.ghost_;
    nextGhost_ = org.nextGhost_;
#endif
    maxlevel_  = org.maxlevel_;

    // copy entity pointer
    // this method will probably free entity
    ALU3dGridEntityPointer<0,GridImp> :: clone(org);
  }

  template <class GridImp>
  inline int
  ALU3dGridHierarchicIterator<GridImp>::
  getLevel(const HBndSegType* face) const
  {
    // return ghost level
    assert( face );
    return face->ghostLevel();
  }

  template <class GridImp>
  inline int
  ALU3dGridHierarchicIterator<GridImp>::
  getLevel(const HElementType * item) const
  {
    // return normal level
    assert( item );
    return item->level();
  }
  template <class GridImp>
  template <class HItemType>
  inline HItemType*
  ALU3dGridHierarchicIterator<GridImp>::
  goNextElement(const HItemType* startElem, HItemType * oldelem )
  {
    // strategy is:
    // - go down as far as possible and then over all children
    // - then go to father and next and down again

    HItemType * nextelem = oldelem->down();
    if(nextelem)
    {
      // use getLevel method
      if( getLevel(nextelem) <= maxlevel_)
        return nextelem;
    }

    nextelem = oldelem->next();
    if(nextelem)
    {
      // use getLevel method
      if( getLevel(nextelem) <= maxlevel_)
        return nextelem;
    }

    nextelem = oldelem->up();
    if(nextelem == startElem) return 0;

    while( !nextelem->next() )
    {
      nextelem = nextelem->up();
      if(nextelem == startElem) return 0;
    }

    if(nextelem) nextelem = nextelem->next();

    return nextelem;
  }

  template <class GridImp>
  inline void ALU3dGridHierarchicIterator<GridImp> :: increment ()
  {
    assert(this->item_ != 0);

#ifdef ALU3DGRID_PARALLEL
    if( ghost_ )
    {
      assert( nextGhost_ );
      nextGhost_ = goNextElement( ghost_, nextGhost_ );
      if( ! nextGhost_ )
      {
        this->done();
        return ;
      }

      this->updateGhostPointer( *nextGhost_ );
    }
    else
#endif
    {
      HElementType * nextItem = goNextElement( elem_, this->item_ );
      if( ! nextItem)
      {
        this->done();
        return ;
      }

      this->updateEntityPointer(nextItem);
    }
    return ;
  }

  template <class GridImp>
  inline typename ALU3dGridHierarchicIterator<GridImp> :: Entity &
  ALU3dGridHierarchicIterator<GridImp> :: dereference () const
  {
    // don't dereference empty entity pointer
    assert( this->item_ );
    assert( this->entity_ );
    assert( this->item_ == & this->entityImp().getItem() );
    return (*this->entity_);
  }




} // end namespace Dune

//#endif // COMPILE_ALUGRID_INLINE_FILES
