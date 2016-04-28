void solveTridiagonalSystem(AccessHandle<std::vector<double>> & subD,
														AccessHandle<std::vector<double>> & diag, 
														AccessHandle<std::vector<double>> & supD,
														AccessHandle<std::vector<double>> & rhs)
{
 	create_work([=]
 	{
 		double * pta = subD->data();
 		double * ptb = diag->data();
 		double * ptc = supD->data();
 		double * ptd = rhs->data();

 		solveThomas(pta, ptb, ptc, ptd, nInn);
 	});
}