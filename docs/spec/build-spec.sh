#!/bin/bash -e

#requires doxygen 1.8.11
doxygen doxygen-backend/Doxyfile
pdflatex sandReportSpec                                                          
makeglossaries sandReportSpec
pdflatex sandReportSpec                                                          
makeglossaries sandReportSpec
pdflatex sandReportSpec                                                          
bibtex sandReportSpec
pdflatex sandReportSpec
pdflatex sandReportSpec
