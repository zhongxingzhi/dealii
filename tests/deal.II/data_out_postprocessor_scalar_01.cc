//----------------------------  data_out.cc  ---------------------------
//    $Id$
//    Version: $Name$
//
//    Copyright (C) 2000, 2001, 2002, 2003, 2004, 2007, 2008, 2011 by the deal.II authors
//
//    This file is subject to QPL and may not be  distributed
//    without copyright and license information. Please refer
//    to the file deal.II/doc/license.html for the  text  and
//    further information on this license.
//
//----------------------------  data_out.cc  ---------------------------

// tests DataPostprocessor: create a FE field that has two components of
// the kind cos(something) and sin(something) and then have a postprocessor
// that computes the sum of squares. should always be equal to one
//
// this test uses the shortcut class DataPostprocessorScalar to make
// writing postprocessors simpler


#include "../tests.h"
#include <deal.II/grid/tria.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria_accessor.h>
#include <deal.II/grid/tria_iterator.h>
#include <deal.II/dofs/dof_accessor.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_system.h>
#include <deal.II/base/function.h>
#include <deal.II/numerics/vectors.h>
#include <deal.II/numerics/matrices.h>
#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/data_postprocessor.h>
#include <fstream>

#include <deal.II/base/logstream.h>


std::ofstream logfile("data_out_postprocessor_scalar_01/output");


template <int dim>
class LaplaceProblem
{
  public:
    LaplaceProblem ();
    void run ();

  private:
    void make_grid_and_dofs ();
    void solve ();
    void output_results () const;

    Triangulation<dim>   triangulation;
    FESystem<dim>            fe;
    DoFHandler<dim>      dof_handler;

    Vector<double>       solution;
};


template <int dim>
LaplaceProblem<dim>::LaplaceProblem ()
		:
		fe (FE_Q<dim>(1),2),
		dof_handler (triangulation)
{}



template <int dim>
void LaplaceProblem<dim>::make_grid_and_dofs ()
{
  GridGenerator::hyper_cube (triangulation, 0, 1);
  triangulation.refine_global (1);
  triangulation.begin_active()->set_refine_flag ();
  triangulation.execute_coarsening_and_refinement ();

  dof_handler.distribute_dofs (fe);
  solution.reinit (dof_handler.n_dofs());
}



template <int dim>
class SinesAndCosines : public Function<dim>
{
  public:
    SinesAndCosines ()
		    :
		    Function<dim> (2)
      {}

    double value (const Point<dim> &p,
		  const unsigned int component) const
      {
	switch (component)
	  {
	    case 0:
		  return std::sin (p.norm());
	    case 1:
		  return std::cos (p.norm());
	    default:
		  Assert (false, ExcNotImplemented());
		  return 0;
	  }
      }
};



template <int dim>
void LaplaceProblem<dim>::solve ()
{
				   // dummy solve. just insert some
				   // values as mentioned at the top
				   // of the file
  VectorTools::interpolate (dof_handler,
			    SinesAndCosines<dim>(),
			    solution);
}


template <int dim>
class MyPostprocessor : public DataPostprocessorScalar<dim>
{
  public:
    MyPostprocessor ()
		    :
		    DataPostprocessorScalar<dim> ("magnitude", update_values)
      {}

    virtual
    void
    compute_derived_quantities_vector (const std::vector<Vector<double> >              &uh,
				       const std::vector<std::vector<Tensor<1,dim> > > &,
				       const std::vector<std::vector<Tensor<2,dim> > > &,
				       const std::vector<Point<dim> >                  &,
				       const std::vector<Point<dim> >                  &,
				       std::vector<Vector<double> >                    &computed_quantities) const
      {
	for (unsigned int q=0; q<uh.size(); ++q)
	  {
	    Assert (computed_quantities[q].size() == 1,
		    ExcInternalError());

	    computed_quantities[q](0) = uh[q](0)*uh[q](0) + uh[q](1)*uh[q](1);
	    Assert (std::fabs(computed_quantities[q](0)-1) < 1e-12,
		    ExcInternalError());
	  }
      }
};



template <int dim>
void LaplaceProblem<dim>::output_results () const
{
  MyPostprocessor<dim> p;
  DataOut<dim> data_out;
  data_out.attach_dof_handler (dof_handler);
  data_out.add_data_vector (solution, p);
  data_out.build_patches ();
  data_out.write_gnuplot (logfile);
}



template <int dim>
void LaplaceProblem<dim>::run ()
{
  make_grid_and_dofs();
  solve ();
  output_results ();
}



int main ()
{
  deallog.depth_console (0);
  logfile << std::setprecision(2);
  deallog << std::setprecision(2);

  LaplaceProblem<2> laplace_problem_2d;
  laplace_problem_2d.run ();

  LaplaceProblem<3> laplace_problem_3d;
  laplace_problem_3d.run ();

  return 0;
}