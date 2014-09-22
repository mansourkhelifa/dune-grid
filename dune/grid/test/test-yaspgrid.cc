// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
// $Id$

#include <config.h>

#include <iostream>
#include <fstream>
#include <strstream>

#include <dune/common/parallel/mpihelper.hh>
#include <dune/grid/yaspgrid.hh>

#include "gridcheck.cc"
#include "checkcommunicate.cc"
#include "checkgeometryinfather.cc"
#include "checkintersectionit.cc"
#include "checkiterators.cc"
#include "checkadaptation.cc"
#include "checkpartition.cc"

template<int dim, class CC>
struct YaspFactory
{};

template<int dim>
struct YaspFactory<dim, Dune::EquidistantCoordinateContainer<double,dim> >
{
  static Dune::YaspGrid<dim>* buildGrid()
  {
    std::cout << " using equidistant coordinate container!" << std::endl << std::endl;

    Dune::FieldVector<double,dim> Len(1.0);
    Dune::array<int,dim> s;
    std::fill(s.begin(), s.end(), 8);
    std::bitset<dim> p(0);
    int overlap = 1;

#if HAVE_MPI
    return new Dune::YaspGrid<dim>(MPI_COMM_WORLD,Len,s,p,overlap);
#else
    return new Dune::YaspGrid<dim>(Len,s,p,overlap);
#endif
  }
};

template<int dim>
struct YaspFactory<dim, Dune::TensorProductCoordinateContainer<double,dim> >
{
  static Dune::YaspGrid<dim, Dune::TensorProductCoordinateContainer<double,dim> >* buildGrid()
  {
    std::cout << " using tensorproduct coordinate container!" << std::endl << std::endl;

    std::bitset<dim> p(0);
    int overlap = 1;

    Dune::array<std::vector<double>,dim> coords;
    for (int i=0; i<dim; i++)
    {
      coords[i].resize(9);
      coords[i][0] = -1.0;
      coords[i][1] = -0.5;
      coords[i][2] = -0.25;
      coords[i][3] = -0.125;
      coords[i][4] =  0.0;
      coords[i][5] =  0.125;
      coords[i][6] =  0.25;
      coords[i][7] =  0.5;
      coords[i][8] =  1.0;
    }

#if HAVE_MPI
    return new Dune::YaspGrid<dim, Dune::TensorProductCoordinateContainer<double,dim> >(MPI_COMM_WORLD,coords,p,overlap);
#else
    return new Dune::YaspGrid<dim, Dune::TensorProductCoordinateContainer<double,dim> >(coords,p,overlap);
#endif
  }
};

template <int dim, class CC = Dune::EquidistantCoordinateContainer<double,dim> >
void check_yasp() {
  std::cout << std::endl << "YaspGrid<" << dim << ">";

  Dune::YaspGrid<dim,CC>* grid = YaspFactory<dim,CC>::buildGrid();

  gridcheck(*grid);
  grid->globalRefine(2);

  gridcheck(*grid);

  checkIterators ( grid->leafGridView() );
  checkIterators ( grid->levelGridView(0) );

  // check communication interface
  checkCommunication(*grid,-1,Dune::dvverb);
  for(int l=0; l<=grid->maxLevel(); ++l)
    checkCommunication(*grid,l,Dune::dvverb);

  // check geometry lifetime
  checkGeometryLifetime( grid->leafGridView() );
  // check the method geometryInFather()
  checkGeometryInFather(*grid);
  // check the intersection iterator and the geometries it returns
  checkIntersectionIterator(*grid);
  // check grid adaptation interface
  checkAdaptRefinement(*grid);
  checkPartitionType( grid->leafGridView() );

  std::ofstream file;
  std::ostrstream filename;
  filename << "output" <<grid->comm().rank();
  file.open(filename.str());
  file << *grid << std::endl;
  file.close();

  delete grid;
}

int main (int argc , char **argv) {
  try {
    // Initialize MPI, if present
    Dune::MPIHelper::instance(argc, argv);

    check_yasp<1>();
    check_yasp<1, Dune::TensorProductCoordinateContainer<double,1> >();

    check_yasp<2>();
    check_yasp<2, Dune::TensorProductCoordinateContainer<double,2> >();

    check_yasp<3>();
    check_yasp<3, Dune::TensorProductCoordinateContainer<double,3> >();

  } catch (Dune::Exception &e) {
    std::cerr << e << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "Generic exception!" << std::endl;
    return 2;
  }

  return 0;
}
