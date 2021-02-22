.PHONY: all clean

all: documentation.html README.rst example_data/example_documents.json

documentation.html: documentation.md
	pandoc $< -s --toc -N -o $@

README.rst: documentation.md
	pandoc documentation.md -t rst --toc -s | \
	  sed 's/:Author:/..\n    Author: /;/\.\. raw/,/ *<\/style>/d' > $@

example_data/documentation.txt: documentation.html
	lynx -dump -nolist -nonumbers -width=70 $< | \
    sed '/jerome@dockes.org/s/$$/\n/;1,/^ *1 Intro/{s/^\( *\)[*+o]/\1 /}'  > $@

example_data/example_documents.json: example_data/documentation.txt example_data/make_example_docs.py example_data/hello_annotations.txt
	python3 example_data/make_example_docs.py

clean:
	rm -f documentation.html README.rst \
    example_data/documentation.txt example_data/example_documents.json
