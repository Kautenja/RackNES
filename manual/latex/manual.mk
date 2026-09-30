# Shared manual build: source-relative inputs, fail on errors, no shell escape.
.DEFAULT_GOAL := manual
.PHONY: manual clean
BUILD ?= .build
LATEXMK ?= latexmk
export TEXINPUTS := ../latex//:$(TEXINPUTS):
manual:
	@test ! -e "$(BUILD)/manual.tex" || { echo "BUILD contains a legacy source copy; choose a fresh output directory." >&2; exit 1; }
	$(LATEXMK) -pdf -pdflatex='pdflatex -no-shell-escape %O %S' -interaction=nonstopmode -halt-on-error -file-line-error -outdir="$(BUILD)" manual.tex
	$(LATEXMK) -c -outdir="$(BUILD)" manual.tex
clean:
	$(LATEXMK) -C -outdir="$(BUILD)" manual.tex
