// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include <config.h>

// compile surface grid support into the lib even for ALBERTA 2.0
#define DUNE_ALBERTA_SURFACE_GRID 1

#include <dune/grid/albertagrid/dgfparser.hh>

#if HAVE_ALBERTA

namespace Dune
{

  // Implementation of DGFGridFactory for AlbertaGrid
  // ------------------------------------------------

  template< int dim, int dimworld >
  bool DGFGridFactory< AlbertaGrid< dim, dimworld > >::generate( std::istream &input )
  {
    dgf_.element = DuneGridFormatParser::Simplex;
    dgf_.dimgrid = dim;
    dgf_.dimw = dimworld;

    if( !dgf_.readDuneGrid( input, dim, dimworld ) )
      return false;

    for( int n = 0; n < dgf_.nofvtx; ++n )
    {
      typename GridFactory::WorldVector coord;
      for( int i = 0; i < dimworld; ++i )
        coord[ i ] = dgf_.vtx[ n ][ i ];
      factory_.insertVertex( coord );
    }

    std::vector< unsigned int > elementId( dimension+1 );
    for( int n = 0; n < dgf_.nofelements; ++n )
    {
      // This is a nasty hack: The tetrahedrons generated by make6 are not
      // directly useable by ALBERTA. On every second tetrahedron we have to
      // switch the last 2 vertices (otherwise ALBERTA causes a segmentation
      // fault during refinement).
      if( (dimension == 3) && dgf_.cube2simplex && (n % 2 == 0) )
      {
        const int flip[ 4 ] = { 0, 1, 3, 2 };
        for( int i = 0; i <= dimension; ++i )
          elementId[ i ] = dgf_.elements[ n ][ flip[ i ] ];
      }
      else
      {
        for( int i = 0; i <= dimension; ++i )
          elementId[ i ] = dgf_.elements[ n ][ i ];
      }

      typedef typename GenericGeometry::SimplexTopology< dimension >::type Topology;
      factory_.insertElement( GeometryType( Topology() ), elementId );

      // look for bounaries and insert them
      for( int face = 0; face <= dimension; ++face )
      {
        typedef typename DuneGridFormatParser::facemap_t::key_type Key;
        typedef typename DuneGridFormatParser::facemap_t::iterator Iterator;

        const Key key( elementId, dimension, face+1 );
        const Iterator it = dgf_.facemap.find( key );
        if( it != dgf_.facemap.end() )
          factory_.insertBoundary( n, face, it->second.first );
      }
    }

    if( GridFactory::supportPeriodicity )
    {
      typedef dgf::PeriodicFaceTransformationBlock::AffineTransformation Transformation;
      dgf::PeriodicFaceTransformationBlock block( input, dimworld );
      const int size = block.numTransformations();
      for( int k = 0; k < size; ++k )
      {
        const Transformation &trafo = block.transformation( k );

        typename GridFactory::WorldMatrix matrix;
        for( int i = 0; i < dimworld; ++i )
          for( int j = 0; j < dimworld; ++j )
            matrix[ i ][ j ] = trafo.matrix( i, j );

        typename GridFactory::WorldVector shift;
        for( int i = 0; i < dimworld; ++i )
          shift[ i ] = trafo.shift[ i ];

        factory_.insertFaceTransformation( matrix, shift );
      }
    }

    dgf::ProjectionBlock projectionBlock( input, dimworld );
    const DuneBoundaryProjection< dimworld > *projection
      = projectionBlock.template defaultProjection< dimworld >();
    if( projection != 0 )
      factory_.insertBoundaryProjection( projection );
    const size_t numBoundaryProjections = projectionBlock.numBoundaryProjections();
    for( size_t i = 0; i < numBoundaryProjections; ++i )
    {
      const std::vector< unsigned int > &vertices = projectionBlock.boundaryFace( i );
      const DuneBoundaryProjection< dimworld > *projection
        = projectionBlock.template boundaryProjection< dimworld >( i );
      typedef typename GenericGeometry::SimplexTopology< dimension-1 >::type Topology;
      factory_.insertBoundaryProjection( GeometryType( Topology() ), vertices, projection );
    }

    dgf::GridParameterBlock parameter( input );
    if( parameter.markLongestEdge() )
      factory_.markLongestEdge();

    const std::string &dumpFileName = parameter.dumpFileName();
    if( !dumpFileName.empty() )
      factory_.write( dumpFileName );

    grid_ = factory_.createGrid();
    return true;
  }



  // Instantiation
  // -------------

  template struct DGFGridFactory< AlbertaGrid< 1, Alberta::dimWorld > >;
#if ALBERTA_DIM >= 2
  template struct DGFGridFactory< AlbertaGrid< 2, Alberta::dimWorld > >;
#endif // #if ALBERTA_DIM >= 2
#if ALBERTA_DIM >= 3
  template struct DGFGridFactory< AlbertaGrid< 3, Alberta::dimWorld > >;
#endif // #if ALBERTA_DIM >= 3

}

#endif // #if HAVE_ALBERTA
