// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_GENERICQUADRATURE_HH
#define DUNE_GENERICQUADRATURE_HH

#include <vector>

#include <dune/common/fvector.hh>
#include <dune/common/geometrytype.hh>

#include <dune/grid/common/topologyfactory.hh>
#include <dune/grid/genericgeometry/conversion.hh>
#include <dune/grid/genericgeometry/topologytypes.hh>

//#include <dune/localfunctions/utility/field.hh>

namespace Dune
{

  namespace GenericGeometry
  {

    // QuadraturePoint
    // ---------------

    /**
     * @brief Base class for a single quadrature point (point and weight)
     *        with template field type and dimension
     **/
    template< class F, unsigned int dim >
    class QuadraturePoint
    {
      typedef QuadraturePoint< F, dim > This;

    public:
      static const unsigned int dimension = dim;

      typedef F Field;

      typedef FieldVector< Field, dimension > Vector;

    private:
      Vector point_;
      Field weight_;

    public:
      QuadraturePoint ( const Vector &point, const Field &weight )
        : point_( point ),
          weight_( weight )
      {}

#if 0
      template <class FF>
      QuadraturePoint ( const QuadraturePoint<FF,dim>& other )
        : point_( field_cast<F>(other.point() ) ),
          weight_( field_cast<F>(other.weight() ) )
      {}
#endif

      const Vector &point () const
      {
        return point_;
      }

      const Field &weight () const
      {
        return weight_;
      }

    };


    // Quadrature
    // ----------

    /**
     * @brief Base class for the generic quadrature implementations.
     *        Field type and dimension are
     *        template argument construction is through a topology id.
     **/
    template< class F, unsigned int dim >
    class Quadrature
    {
      typedef Quadrature< F, dim > This;

    public:
      typedef F Field;
      static const unsigned int dimension = dim;

      typedef GenericGeometry::QuadraturePoint< Field, dimension > QuadraturePoint;
      typedef typename QuadraturePoint::Vector Vector;

      typedef typename std::vector< QuadraturePoint >::const_iterator Iterator;

    public:
      //! Constructor taking topology id
      explicit Quadrature ( const unsigned int topologyId )
        : topologyId_( topologyId )
      {}

      //! Copy constructor
      template< class Q >
      Quadrature ( const Q &q )
        : topologyId_( q.topologyId() )
      {
        points_.reserve( q.size() );
        const typename Q::Iterator end = q.end();
        for( typename Q::Iterator it = q.begin(); it != end; ++it )
          points_.push_back( *it );
      }

      //! Access a quadrature point
      const QuadraturePoint &operator[] ( const unsigned int i ) const
      {
        return points_[ i ];
      }

      //! start iterator over the quadrature points
      Iterator begin () const
      {
        return points_.begin();
      }

      //! end iterator over the quadrature points
      Iterator end () const
      {
        return points_.end();
      }

      //! access the coordinates of a quadrature point
      const Vector &point ( const unsigned int i ) const
      {
        return (*this)[ i ].point();
      }

      //! access the weight of a quadrature point
      const Field &weight ( const unsigned int i ) const
      {
        return (*this)[ i ].weight();
      }

      //! topology id of the quadrature
      unsigned int topologyId () const
      {
        return topologyId_;
      }

      //! number of quadrature points
      size_t size () const
      {
        return points_.size();
      }

    protected:
      void insert ( const QuadraturePoint &point )
      {
        points_.push_back( point );
      }

      void insert ( const Vector &position, const Field &weight )
      {
        insert( QuadraturePoint( position, weight ) );
      }

      template< unsigned int d, class QC >
      friend struct SubQuadratureCreator;

    private:
      std::vector< QuadraturePoint > points_;
      unsigned int topologyId_;
    };



    // GenericQuadrature
    // -----------------

    /**
     * @brief extends a 1d quadrature to a generic reference elemenet
     *
     * \tparam Topology the topology of the reference element
     * \tparam OneDQuad a quadrature for [0,1]
     *
     * The 1d quadrature must be a std::vector-like container with a constructor
     * taking an order parameter
     **/
    template< class Topology, class OneDQuad >
    class GenericQuadrature;

    /** \brief Generic Quadrature for Point
    **/
    template< class OneDQuad >
    class GenericQuadrature< Point, OneDQuad >
      : public Quadrature< typename OneDQuad::Field, 0 >
    {
      typedef typename OneDQuad::Field F;
      typedef GenericQuadrature< Point, OneDQuad > This;
      typedef Quadrature< F, 0 > Base;

    public:
      typedef Point Topology;

      typedef F Field;
      static const unsigned int dimension = Base::dimension;

      typedef typename Base::Vector Vector;

      explicit GenericQuadrature ( unsigned int order )
        : Base( Topology::id )
      {
        Base::insert( Vector( Field( 0 ) ), 1 );
      }
    };


    /** \brief Generic Quadrature for Prisms
    **/
    template< class BaseTopology, class OneDQuad >
    class GenericQuadrature< Prism< BaseTopology >, OneDQuad >
      : public Quadrature< typename OneDQuad::Field, Prism< BaseTopology >::dimension >
    {
      typedef typename OneDQuad::Field F;
      typedef GenericQuadrature< Prism< BaseTopology >, OneDQuad > This;
      typedef Quadrature< F, Prism< BaseTopology >::dimension > Base;

    public:
      typedef Prism< BaseTopology > Topology;

      typedef F Field;
      static const unsigned int dimension = Base::dimension;

      typedef typename Base::Vector Vector;

    private:
      typedef OneDQuad OneDQuadrature;
      typedef GenericQuadrature< BaseTopology, OneDQuadrature > BaseQuadrature;

    public:
      explicit GenericQuadrature ( unsigned int order )
        : Base( Topology::id )
      {
        OneDQuadrature onedQuad( OneDQuadrature::minPoints(order) );
        BaseQuadrature baseQuad( order );

        const unsigned int baseQuadSize = baseQuad.size();
        for( unsigned int bqi = 0; bqi < baseQuadSize; ++bqi )
        {
          const typename BaseQuadrature::Vector &basePoint = baseQuad[bqi].point( );
          const Field &baseWeight = baseQuad[bqi].weight( );

          Vector point;
          for( unsigned int i = 0; i < dimension-1; ++i )
            point[ i ] = basePoint[ i ];

          const unsigned int onedQuadSize = onedQuad.size();
          for( unsigned int oqi = 0; oqi < onedQuadSize; ++oqi )
          {
            point[ dimension-1 ] = onedQuad[oqi].point()[ 0 ];
            Base::insert( point, baseWeight * onedQuad[oqi].weight( ) );
          }
        }
      }
    };


    /** \brief Generic Quadrature for Pyramids
     *
     *  This quadrature for \f$B^\circ\f$ is generated from a quadrature for
     *  \f$B\f$ and a 1D quadrature by the so-called Duffy-Transformation
     *  \f$y(x,z) = ((1-z)x,y)^T\f$. Hence, we have
     *  \f[
     *  \int_{B^\circ} f( y )\,\mathrm{d}y
     *  = \int_0^1 \int_B f( (1-z)x, z )\,\mathrm{d}x\,(1-z)^{\dim B}\,\mathrm{d}z.
     *  \f]
     *  Therefore, the 1D quadrature must be at least \f$\dim B\f$ orders higher
     *  than the quadrature for \f$B\f$.
     *
     *  Question: If the polynomials are created via Duffy Transformation, do we
     *            really need a higher quadrature order?
     */
    template< class BaseTopology, class OneDQuad >
    class GenericQuadrature< Pyramid< BaseTopology >, OneDQuad >
      : public Quadrature< typename OneDQuad::Field, Pyramid< BaseTopology >::dimension >
    {
      typedef typename OneDQuad::Field F;
      typedef GenericQuadrature< Pyramid< BaseTopology >, OneDQuad > This;
      typedef Quadrature< F, Pyramid< BaseTopology >::dimension > Base;

    public:
      typedef Pyramid< BaseTopology > Topology;

      typedef F Field;
      static const unsigned int dimension = Base::dimension;

      typedef typename Base::Vector Vector;

    private:
      typedef OneDQuad OneDQuadrature;
      typedef GenericQuadrature< BaseTopology, OneDQuadrature > BaseQuadrature;

    public:
      explicit GenericQuadrature ( unsigned int order )
        : Base( Topology::id )
      {
        OneDQuadrature onedQuad( OneDQuadrature::minPoints(order + dimension-1 ) );
        BaseQuadrature baseQuad( order );

        const unsigned int baseQuadSize = baseQuad.size();
        for( unsigned int bqi = 0; bqi < baseQuadSize; ++bqi )
        {
          const typename BaseQuadrature::Vector &basePoint = baseQuad[bqi].point( );
          const Field &baseWeight = baseQuad[bqi].weight( );

          const unsigned int onedQuadSize = onedQuad.size();
          for( unsigned int oqi = 0; oqi < onedQuadSize; ++oqi )
          {
            Vector point;
            point[ dimension-1 ] = onedQuad[oqi].point( )[ 0 ];
            const Field scale = Field( 1 ) - point[ dimension-1 ];
            for( unsigned int i = 0; i < dimension-1; ++i )
              point[ i ] = scale * basePoint[ i ];

            Field weight = baseWeight * onedQuad[oqi].weight( );
            for ( unsigned int p = 0; p<dimension-1; ++p)
              weight *= scale;                    // pow( scale, dimension-1 );
            Base::insert( point, weight );
          }
        }
      }
    };


    /**
     * @brief Factory for the generic quadratures
     *
     * This is a Dune::GenericGeometry::TopologyFactory creating
     * GenericQuadrature from a given 1d quadrature
     *
     * \tparam dim dimension of the reference elements contained in the factory
     * \tparam F field in which weight and point of the quadrature are stored
     * \tparam OneDQuad the underlying 1d quadrature
     *
     * Note: the computation of the quadrature points and weights are
     * carried out in the field type of the 1d quadrature which can differ from F.
     **/
    template< int dim, class F, class OneDQuad >
    struct GenericQuadratureFactory;

    template< int dim, class F, class OneDQuad >
    struct GenericQuadratureFactoryTraits
    {
      static const unsigned int dimension = dim;
      typedef unsigned int Key;
      typedef const Quadrature<F,dim> Object;
      typedef GenericQuadratureFactory<dim,F,OneDQuad> Factory;
    };

    template< int dim, class F, class OneDQuad >
    struct GenericQuadratureFactory :
      public TopologyFactory< GenericQuadratureFactoryTraits<dim,F,OneDQuad> >
    {
      static const unsigned int dimension = dim;
      typedef F Field;
      typedef GenericQuadratureFactoryTraits<dim,F,OneDQuad> Traits;

      typedef typename Traits::Key Key;
      typedef typename Traits::Object Object;

      template< class Topology >
      static Object* createObject ( const Key &order )
      {
        return new Object( GenericQuadrature< Topology, OneDQuad >( order ) );
      }
    };


    // GenericQuadratureProvider
    // ---------------------

    /**
     * @brief Singleton factory for the generic quadratures
     *
     * Wrapper for the Dune::GenericGeometry::GenericQuadratureFactory providing
     * singleton storage.
     *
     * \tparam dim dimension of the reference elements contained in the factory
     * \tparam F field in which weight and point of the quadrature are stored
     * \tparam OneDQuad the underlying 1d quadrature
     **/
    template< int dim, class F, class OneDQuad >
    struct GenericQuadratureProvider
      : public TopologySingletonFactory< GenericQuadratureFactory< dim, F, OneDQuad > >
    {};

  }

}

#endif // #ifndef DUNE_GENERICQUADRATURE_HH