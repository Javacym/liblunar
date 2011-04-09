PWD=$(shell pwd)
all: lunar-date-doc lunar-calendar-doc
lunar-date-doc:
	@mkdir tmp; \
		svn export https://liblunar.googlecode.com/svn/trunk/lunar-date tmp/lunar-date; \
		cd tmp/lunar-date/; \
		./autogen.sh --enable-gtk-doc; \
		make; \
		cp docs/reference/html/* ${PWD}/lunar-date/; \
		cd ${PWD}/lunar-date; \
		for i in *.html;do svn propset "svn:mime-type" "text/html" $$i;done; \
		rm -rf tmp
lunar-calendar-doc:
	@mkdir tmp; \
		svn export https://liblunar.googlecode.com/svn/trunk/lunar-calendar tmp/lunar-calendar; \
		cd tmp/lunar-calendar/; \
		./autogen.sh --enable-gtk-doc; \
		make; \
		cp docs/reference/html/* ${PWD}/lunar-calendar/; \
		cd ${PWD}/lunar-calendar; \
		for i in *.html;do svn propset "svn:mime-type" "text/html" $$i;done; \
		rm -rf tmp
