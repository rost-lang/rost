
all: rost.pdf rost.html

%.pdf: %.texi
	texi2pdf $<

%.html: %.texi
	makeinfo --html --ifhtml --force --no-split --output=$@ $<

clean:
	rm -f rost.aux rost.cp rost.fn rost.ky rost.log rost.pdf \
          rost.html rost.pg rost.toc rost.tp rost.vr