#!/bin/bash                                                                 


doxygen doxygen-backend/Doxyfile
pdflatex sandReportSpec                                                          
makeglossaries sandReportSpec
pdflatex sandReportSpec                                                          
makeglossaries sandReportSpec
pdflatex sandReportSpec                                                          
bibtex sandReportSpec
pdflatex sandReportSpec
pdflatex sandReportSpec
