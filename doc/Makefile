all: vidthumb.1

%.1: %.xml Makefile
	echo "<!ENTITY vidthumb_version \"$$(cat ../VERSION)\">" > vidthumb.ent
	db2x_xsltproc --stylesheet man "$<" | db2x_manxml

# EOF #

