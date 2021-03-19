html := $(patsubst %.adoc,%.html,$(wildcard *.adoc))
html := $(subst README.html,documentation.html,$(html))
webhtml := $(patsubst %,gh-pages-site/%,$(html))
examples := $(wildcard example_data/*.txt)

.PHONY: all clean html

all: $(html) $(webhtml) labelbuddy.1 example_data/example_documents.json

%.html: %.adoc ../data/VERSION.txt
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $<

labelbuddy.1: manpage.adoc ../data/VERSION.txt
	asciidoctor -a version="$$(cat ../data/VERSION.txt)" -b manpage $<

gh-pages-site/%.html: %.html
	mkdir -p gh-pages-site
	xsltproc add_nav.xsl $< > $@

gh-pages-site/screenshots.html: screenshots.html
	mkdir -p gh-pages-site
	xsltproc add_nav.xsl $< > $@
	mkdir -p gh-pages-site/screenshots
	cp screenshots/* gh-pages-site/screenshots/

documentation.html: README.adoc ../data/VERSION.txt
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@

example_data/documentation.txt: documentation.html
	lynx -dump -nolist -nonumbers -width=1000 $< | \
    sed '1s/^/html version: Help > documentation\n\n/;s/\xe2\x80\x8b//g;/^ *Last updated[-: 0-9]*$$/d'  > $@

# example_data/documentation.txt: documentation.html
# 	pandoc $<  --wrap=none -t plain | \
#      sed '1s/^/\nThis document is better read with a fixed width font: Preferences > Choose font, or Help > documentation for html version\n\n/;s/\xe2\x80\x8b//g;/^ *Last updated[-: 0-9]*$$/d'  > $@


example_data/example_documents.json: $(examples) example_data/example_labels.json example_data/documentation.txt example_data/make_example_docs.py
	python3 example_data/make_example_docs.py

clean:
	rm -f documentation.html  index.html  installation.html  manpage.html \
    example_data/documentation.txt example_data/example_documents.json
