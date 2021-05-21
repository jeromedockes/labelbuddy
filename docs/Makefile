html := $(patsubst %.adoc,%.html,$(wildcard *.adoc))
html := $(subst README.html,documentation.html,$(html))
html += readme-as-seen-on-github.html
webhtml := $(patsubst %,gh-pages-site/%,$(html))
examples :=$(filter-out %.py %annotations.json, $(wildcard example_data/*))
examples := $(patsubst %.adoc,%.html,$(examples))
examples += example_data/wiki_extracts_documents.json
examples += example_data/pos_labels.json
webexamples := $(patsubst %,gh-pages-site/%, $(examples))
schemas := $(wildcard schema/*)
schemas := $(patsubst %.adoc,%.html,$(schemas))
webschemas := $(patsubst %,gh-pages-site/%, $(schemas))
demo := $(wildcard demo_data/*.txt)
screenshots := $(wildcard screenshots/*.png)

.PHONY: all clean web

all: $(html) labelbuddy.1 demo_data/example_documents.json $(examples) $(schemas)

web: $(webhtml) $(webexamples) $(webschemas)

%.html: %.adoc docinfo.html ../data/VERSION.txt
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $<

labelbuddy.1: manpage.adoc ../data/VERSION.txt
	asciidoctor -a version="$$(cat ../data/VERSION.txt)" -b manpage $<

gh-pages-site/example_data/%: example_data/%
	mkdir -p gh-pages-site/example_data
	cp $< $@

gh-pages-site/schema/%: schema/%
	mkdir -p gh-pages-site/schema
	cp $< $@

gh-pages-site/%.html: %.html
	mkdir -p gh-pages-site
	xsltproc add_nav.xsl $< > $@

gh-pages-site/screenshots.html: screenshots.html $(screenshots)
	mkdir -p gh-pages-site
	xsltproc add_nav.xsl $< > $@
	mkdir -p gh-pages-site/screenshots
	cp screenshots/* gh-pages-site/screenshots/ 2>/dev/null || :

example_data/exported_docs.json: example_data/make_examples.py
	$<

documentation.html: README.adoc docinfo.html ../data/VERSION.txt example_data/exported_docs.json
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@

readme-as-seen-on-github.html: README.adoc docinfo.html ../data/VERSION.txt example_data/exported_docs.json
	asciidoctor -b xhtml -a env-github -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@


demo_data/example_documents.json: $(demo) demo_data/example_labels.json demo_data/make_example_docs.py
	python3 demo_data/make_example_docs.py

demo_data/example_documents_for_website.json: $(demo) demo_data/make_example_docs.py
	python3 demo_data/make_example_docs.py

example_data/wiki_extracts_documents.json: demo_data/example_documents_for_website.json
	cp $< $@

example_data/pos_labels.json: demo_data/example_labels.json
	cp $< $@

clean:
	rm -f documentation.html  index.html  installation.html  manpage.html \
    demo_data/example_documents.json demo_data/example_documents_for_website.json \
    labelbuddy.1 readme-as-seen-on-github.html \
    screenshots.html example_data/*.xml example_data/*.json \
    example_data/*.jsonl example_data/*.csv example_data/*.txt example_data/*.html \
    schema/*.html
	rm -rf gh-pages-site
