html := $(patsubst %.adoc,%.html,$(wildcard *.adoc))

.PHONY: all clean html

all: $(html)

%.html: %.adoc
	asciidoctor -a lbversion="$$(cat VERSION.txt)" $<

clean:
	rm -f *.html
