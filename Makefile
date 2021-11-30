
all:
	pdflatex paper.tex
	bibtex paper
	pdflatex paper.tex
	pdflatex paper.tex

paper.pdf: paper.tex paper.bib
	make all

final: paper.pdf frontpage.pdf
	pdfunite frontpage.pdf paper.pdf final.pdf

clean:
	rm -rf paper.aux paper.bbl paper.blg paper.log paper.out