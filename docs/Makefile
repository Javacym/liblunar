PWD=`pwd`
all: lunar-date-doc
lunar-date-doc:
	@mkdir tmp; \
		svn export https://liblunar.googlecode.com/svn/trunk/liblunar tmp/liblunar; \
		cd tmp/liblunar/; \
		./autogen.sh --enable-gtk-doc; \
		make; \
		cp docs/reference/html/* ${PWD}/lunar-date/; \
		cd ${PWD}/lunar-date; \
		for i in *.html;do svn propset "svn:mime-type" "text/html" $$i;done; \
		rm -rf tmp
