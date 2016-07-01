struct solveTridiagonalSystem{
  void operator()(std::vector<double> & subD,
                  std::vector<double> & diag, 
                  std::vector<double> & supD,
                  std::vector<double> & rhs) const{
 		solveThomas(subD.data(), diag.data(), supD.data(), rhs.data(), nInn);
  }
};