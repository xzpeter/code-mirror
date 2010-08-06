SRC=$(shell find . -name "*.tex")

PHONY : pdf clean

pdf : main.pdf

main.pdf : $(SRC)
	xelatex main.tex

clean : 
	@rm *.aux *.log *.out *.pdf *.toc
