
MIN=		.min

# jQuery TreeTable widget
TT_BASEDIR=	../../../treeTable/treeTable/src
TT_css=		$(TT_BASEDIR)/stylesheets
TT_js=		$(TT_BASEDIR)/javascripts
TT_png=		$(TT_BASEDIR)/images
TT_FILES=	$(notdir $(wildcard $(TT_png)/*.png) \
			 $(TT_css)/jquery.treeTable.css \
			 $(TT_js)/jquery.treeTable$(MIN).js)

# Images from GNOME
JQTT_gif=	/home/gnb/software/jqtreetable/images
JQTT_FILES=	new.gif fopen.gif fshut.gif

CSS_FILES=	ggcov.css
JS_FILES=	ggcov.js jquery-1.4.4$(MIN).js
HTML_FILES=	index.html
IMAGE_FILES=	favicon.ico ggcov32.png
CGI_FILES=	ggcov.cgi .htaccess
ALL_FILES=	$(CSS_FILES) $(JS_FILES) $(HTML_FILES) $(IMAGE_FILES) \
		$(CGI_FILES) $(TT_FILES) $(JQTT_FILES)

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

$(filter %.png,$(TT_FILES)) : %.png : $(TT_png)/%.png
	ln -s $< $@

$(filter %.js,$(TT_FILES)) : %.js : $(TT_js)/%.js
	ln -s $< $@

$(filter %.css,$(TT_FILES)) : %.css : $(TT_css)/%.css
	sed -e 's|\.\./images/||g' < $< > $@

$(filter %.gif,$(JQTT_FILES)) : %.gif : $(JQTT_gif)/%.gif
	ln -s $< $@

clean:
	$(RM) ggcov.cgi ggcov32.png $(TT_FILES) $(JQTT_FILES)

# %.css: %.css.in
# 	m4 $(M4FLAGS) $< > $@

