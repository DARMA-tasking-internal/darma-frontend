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