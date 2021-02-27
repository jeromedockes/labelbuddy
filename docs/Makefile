html := $(patsubst %.adoc,%.html,$(wildcard *.adoc))
html := $(subst README.html,documentation.html,$(html))
webhtml := $(patsubst %,gh-pages-site/%,$(html))
examples := $(wildcard example_data/*.txt)

.PHONY: all clean html

all: $(html) $(webhtml) example_data/example_documents.json

%.html: %.adoc ../data/VERSION.txt
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $<

gh-pages-site/%.html: %.html
	mkdir -p gh-pages-site
	xsltproc add_nav.xsl $< > $@

documentation.html: README.adoc ../data/VERSION.txt
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@

# example_data/documentation.txt: documentation.html
# 	lynx -dump -nolist -nonumbers -width=79 $< | \
#     sed 's/\xe2\x80\x8b//g;/^ *Last updated[-: 0-9]*$$/d'  > $@

example_data/documentation.txt: documentation.html
	pandoc $<  -t plain | \
     sed '1s/^/\nThis doc is better read with a fixed width font: Preferences > monospace, or Help > documentation for html version\n\n/;s/\xe2\x80\x8b//g;/^ *Last updated[-: 0-9]*$$/d'  > $@


example_data/example_documents.json: $(examples) example_data/documentation.txt example_data/make_example_docs.py
	python3 example_data/make_example_docs.py

clean:
	rm -f *.html \
    example_data/documentation.txt example_data/example_documents.json
