demo_docs := $(wildcard demo_data/*.txt)
examples := $(shell ./list_examples.sh)
examples += modules/ROOT/examples/annotations.json
gen-attachments := $(patsubst modules/ROOT/examples/%, modules/ROOT/attachments/%, $(examples))
gen-attachments += modules/ROOT/attachments/wiki_extracts_documents.json modules/ROOT/attachments/pos_labels.json

.PHONY: all clean

all: labelbuddy.1 documentation.html keybindings.html antora.yml $(gen-attachments) demo_data/example_documents.json

antora.yml: antora.yml.template ../data/VERSION.txt
	sed 's/LABELBUDDY_VERSION/'"$$(cat ../data/VERSION.txt)"'/g' $< > $@

documentation.html: modules/ROOT/pages/documentation.adoc modules/ROOT/pages/docinfo.html ../data/VERSION.txt $(examples)
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@

keybindings.html: modules/ROOT/pages/keybindings.adoc modules/ROOT/pages/documentation.adoc antora.yml
	asciidoctor -b xhtml -a lbversion="$$(cat ../data/VERSION.txt)" $< -o $@

labelbuddy.1: modules/ROOT/pages/manpage.adoc ../data/VERSION.txt
	asciidoctor -a lbversion="$$(cat ../data/VERSION.txt)" -b manpage $< -o $@

demo_data/%.json: demo_data/make_example_docs.py $(demo_docs)
	$<

modules/ROOT/examples/%: modules/ROOT/examples/make_examples.py
	$<

modules/ROOT/examples/make_examples.py:
	pass

modules/ROOT/attachments/wiki_extracts_documents.json: demo_data/wiki_extracts_documents.json
	cp $< $@

modules/ROOT/attachments/pos_labels.json: demo_data/example_labels.json
	cp $< $@

modules/ROOT/attachments/%: modules/ROOT/examples/%
	cp $< $@

clean:
	rm -f documentation.html labelbuddy.1 antora.yml \
    demo_data/example_documents.json demo_data/wiki_extracts_documents.json \
		$(examples) $(gen-attachments)
