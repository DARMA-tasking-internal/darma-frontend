void checkFinalL1Error(AccessHandle<std::vector<double>> & solution)
{
 	create_work([=]
 	{
 		double * ptd = solution->data();

		double error = 0.0;
		double x = dx;
		for (int i = 0; i < (int) solution->size(); ++i)
		{
			error += std::abs( trueSolution(x) - ptd[i] );
			x += dx;
		}
		std::cout << " L1 error = " << error << std::endl;
		assert( error < 1e-2 );
 	});
}
