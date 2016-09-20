
#ifndef POISSON1DFUNCTORS_H_
#define POISSON1DFUNCTORS_H_

struct initialize{
  void operator()(std::vector<double> & subD,
                  std::vector<double> & diag, 
                  std::vector<double> & supD,
                  std::vector<double> & rhs) const{

    subD.resize(nInn,0.0); diag.resize(nInn,0.0);
    supD.resize(nInn,0.0); rhs.resize(nInn,0.0);
    // loop and set the values based on finite-difference stencil
    double x = dx;
    for (int i = 0; i < nInn; ++i)
    {
      diag[i] = -2; // diagonal elements

      // sub and super diagonals
      if (i>0)
        subD[i] = 1.0;
      if (i<nInn-1)
        supD[i] = 1.0;

      // right hand side
      rhs[i] = rhsEval(x) * dx*dx;  

      // correction to RHS due to known BC
      if (i==0)
        rhs[i] -= BC(xL);
      if (i==nInn-1)
        rhs[i] -= BC(xR);

      x += dx;    
    }
  }
};


struct solveTridiagonalSystem{
  void operator()(std::vector<double> & subD,
                  std::vector<double> & diag, 
                  std::vector<double> & supD,
                  std::vector<double> & rhs) const{
    solveThomas(subD.data(), diag.data(), supD.data(), rhs.data(), nInn);
  }
};


struct checkFinalL1Error{
  void operator()(std::vector<double> & solution) const{

    double error = 0.0;
    double x = dx;
    for (int i = 0; i < (int) solution.size(); ++i)
    {
      error += std::abs( trueSolution(x) - solution[i] );
      x += dx;
    }
    std::cout << " L1 error = " << error << std::endl;
    assert( error < 1e-2 );
  }
};


#endif
