# the default when running `make` in this directory
default: manual

# the source tex file basename (no `.tex`)
SOURCE=manual
# the build directory
BUILD=build

# File-types to clean out from the build folder after pdf compilation
CLEAN=*.aux *.nav *.log *.out *.snm *.toc *.bbl *.blg *.brf *.swp *.nlo *.spl
# a command for running pdflatex with options applied
PDFLATEX=pdflatex -halt-on-error --shell-escape

# make the manual
manual:
	rm -rf ${BUILD}
	mkdir -p ${BUILD}
	cp -r img ${BUILD}
	cp *.tex ${BUILD}
	cd ${BUILD} && ${PDFLATEX} ${SOURCE} | grep -a3 "^!" || true
	cd ${BUILD} && ${PDFLATEX} ${SOURCE} | grep -a3 "^!" || true
	cd ${BUILD} && rm -f ${CLEAN}
