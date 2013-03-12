//---------------------------------------------------------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 2009, 2010, 2013 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//---------------------------------------------------------------------------


// Create a mesh and attach a function to the signal that is triggered
// whenever we refine the mesh. count how often it is called on each
// processor. (note: this is more than once per refinement cycle because we
// have to re-balance and rebuild the mesh.)

#include "../tests.h"
#include <deal.II/base/logstream.h>
#include <deal.II/base/tensor.h>
#include <deal.II/grid/tria.h>
#include <deal.II/lac/vector.h>
#include <deal.II/distributed/tria.h>
#include <deal.II/distributed/grid_refinement.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_out.h>
#include <deal.II/grid/grid_tools.h>
#include <deal.II/base/utilities.h>


#include <fstream>

int counter = 0;
void listener ()
{
  ++counter;
}


void test()
{
  unsigned int myid = Utilities::MPI::this_mpi_process (MPI_COMM_WORLD);

  parallel::distributed::Triangulation<2>
    tr(MPI_COMM_WORLD,
       Triangulation<2>::MeshSmoothing(),
       parallel::distributed::Triangulation<2>::mesh_reconstruction_after_repartitioning);
  GridGenerator::hyper_cube(tr);
  tr.signals.post_refinement.connect (&listener);

  // try some global refinement
  counter = 0;
  tr.refine_global (1);
  if (myid == 0)
    deallog << "refine_global(1) results in a total of " << counter
	    << std::endl;

  counter = 0;
  tr.refine_global (3);
  if (myid == 0)
    deallog << "refine_global(3) results in a total of " << counter
	    << std::endl;


  // now also find the bottom left corner of the domain and, on the processor
  // that owns this cell, refine it 4 times
  for (unsigned int i=0; i<4; ++i)
    {
      counter = 0;
      Triangulation<2>::cell_iterator cell = tr.begin(0);
      while (cell->has_children())
	cell = cell->child(0);
      if (cell->is_locally_owned())
	cell->set_refine_flag();
      tr.execute_coarsening_and_refinement ();
      if (myid == 0)
	deallog << "local refinement results in a total of " << counter
		<< std::endl;
    }
}


int main(int argc, char *argv[])
{
#ifdef DEAL_II_COMPILER_SUPPORTS_MPI
  MPI_Init (&argc,&argv);
#else
  (void)argc;
  (void)argv;
#endif

  unsigned int myid = Utilities::MPI::this_mpi_process (MPI_COMM_WORLD);


  deallog.push(Utilities::int_to_string(myid));

  if (myid == 0)
    {
      std::ofstream logfile(output_file_for_mpi("refinement_listener_02").c_str());
      deallog.attach(logfile);
      deallog.depth_console(0);
      deallog.threshold_double(1.e-10);

      test();
    }
  else
    test();


#ifdef DEAL_II_COMPILER_SUPPORTS_MPI
  MPI_Finalize();
#endif
}