
CSS_FILES=	ggcov.css
JS_FILES=	ggcov.js
HTML_FILES=	index.html
IMAGE_FILES=	favicon.ico ggcov32.png
CGI_FILES=	ggcov.cgi .htaccess
ALL_FILES=	$(CSS_FILES) $(JS_FILES) $(HTML_FILES) $(IMAGE_FILES) $(CGI_FILES)

test_flags=
test_target=    $(HOME)/public_html/ggcovtest/

# install_flags=	-e ssh
# install_target=	gnb@redcape:/home/www/www.fmeh.org/

# test install:
.PHONY: test
test: $(ALL_FILES)
	rsync -rpLv --delete $($@_flags) $(ALL_FILES) $($@_target)

ggcov.cgi:  ../src/ggcov.cgi
	ln -s $< $@

ggcov32.png: ../ui/ggcov32.png
	ln -s $< $@

clean:
	$(RM) ggcov.cgi

# %.css: %.css.in
# 	m4 $(M4FLAGS) $< > $@

