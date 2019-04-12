PROG=bech32
MANSEC=1
OBJS=	$(PROG).o \
	segwit_addr.o

manfmt: $(PROG).$(MANSEC).html $(PROG).$(MANSEC).md $(PROG).$(MANSEC).txt
#manfmt: $(PROG).$(MANSEC).html $(PROG).$(MANSEC).txt

$(PROG).$(MANSEC).html: $(PROG).$(MANSEC)
	mandoc -I os=Bitcoin -T html $< > $@

#Currently broken with message:
#mandoc -I os=Bitcoin -T markdown easyseed.1 > easyseed.1.md
#mandoc: mdoc_markdown.c:333: md_node: Assertion `n->tok >= MDOC_Dd && n->tok < MDOC_MAX' failed.
$(PROG).$(MANSEC).md: $(PROG).$(MANSEC)
	mandoc -I os=Bitcoin -T markdown $< > $@

$(PROG).$(MANSEC).txt: $(PROG).$(MANSEC)
	# XXX: Ridiculous kludge around make/shell quoting issues which
	# prevent me from simply using sed expression $'s/[^\b]\b//g' :
	mandoc -I os=Bitcoin -T utf8 $< | \
		tr '\b' '^' | sed $(SEDRE) -e 's/[^^]\^//g' > $@

clean:
	rm -f $(PROG) $(OBJS) \
		$(PROG).$(MANSEC).html $(PROG).$(MANSEC).md $(PROG).$(MANSEC).txt

#check: $(PROG)
#	./$(PROG) -T
