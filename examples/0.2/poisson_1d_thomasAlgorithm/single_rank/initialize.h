void initialize(AccessHandle<std::vector<double>> & subD,
								AccessHandle<std::vector<double>> & diag, 
								AccessHandle<std::vector<double>> & supD,
								AccessHandle<std::vector<double>> & rhs)
{
 	create_work([=]
 	{
    subD.emplace_value(); 	diag.emplace_value();
    supD.emplace_value();		rhs.emplace_value();
    subD->resize(nInn);			diag->resize(nInn,0.0);
    supD->resize(nInn,0.0);	rhs->resize(nInn,0.0);
    double * ptrD1 = subD->data();	double * ptrD2 = diag->data();
    double * ptrD3 = supD->data();	double * ptrD4 = rhs->data();

    double x = dx;
    for (int i = 0; i < nInn; ++i)
    {
      ptrD2[i] = -2;

      if (i>0)
        ptrD1[i] = 1.0;
      if (i<nInn-1)
        ptrD3[i] = 1.0;

      ptrD4[i] = rhsEval(x) * dx*dx;

      if (i==1)
        ptrD4[i] -= BC(xL);
      if (i==nInn-1)
        ptrD4[i] -= BC(xR);

      x += dx;		
    }
	});	
}