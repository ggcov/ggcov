
MIN=		.min

# jQuery TreeTable widget
JQTT_BASEDIR=	../../../treeTable/treeTable/src
JQTT_css=	$(JQTT_BASEDIR)/stylesheets
JQTT_js=	$(JQTT_BASEDIR)/javascripts
JQTT_png=	$(JQTT_BASEDIR)/images
JQTT_FILES=	$(notdir $(wildcard $(JQTT_png)/*.png) \
			 $(JQTT_css)/jquery.treeTable.css \
			 $(JQTT_js)/jquery.treeTable$(MIN).js)

CSS_FILES=	ggcov.css
JS_FILES=	ggcov.js jquery-1.4.4$(MIN).js
HTML_FILES=	index.html
IMAGE_FILES=	favicon.ico ggcov32.png
CGI_FILES=	ggcov.cgi .htaccess
ALL_FILES=	$(CSS_FILES) $(JS_FILES) $(HTML_FILES) $(IMAGE_FILES) \
		$(CGI_FILES) $(JQTT_FILES)

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

$(filter %.png,$(JQTT_FILES)) : %.png : $(JQTT_png)/%.png
	ln -s $< $@

$(filter %.js,$(JQTT_FILES)) : %.js : $(JQTT_js)/%.js
	ln -s $< $@

$(filter %.css,$(JQTT_FILES)) : %.css : $(JQTT_css)/%.css
	sed -e 's|\.\./images/||g' < $< > $@

clean:
	$(RM) ggcov.cgi ggcov32.png $(JQTT_FILES)

# %.css: %.css.in
# 	m4 $(M4FLAGS) $< > $@

