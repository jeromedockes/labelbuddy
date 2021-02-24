html := $(patsubst %.adoc,%.html,$(wildcard *.adoc))

.PHONY: all clean html

all: $(html)

%.html: %.adoc
	asciidoctor $<

clean:
	rm -f *.html
