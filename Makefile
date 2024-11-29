BASE_URL =
BASE_PATH =

SITE = _site

FILES = $(shell find md -type f -name \*.md)
PAGES = $(patsubst %.md, %/index.html, $(patsubst %/index.md, %/index.html, $(FILES)))

IMAGES = $(patsubst svg/%.svg, img/%.png, $(wildcard svg/*.svg))
CONTENT = $(addprefix $(SITE)/, css etc img) $(patsubst md/%, $(SITE)/%, $(PAGES))

PANDOC = pandoc -d defaults.yml -M base_url=$(BASE_URL) -M base_path=$(BASE_PATH)
INKSCAPE = inkscape --export-area-page --export-dpi=144 --export-type=png --export-filename=-

MKDIR = mkdir -p
CP = cp -r
RM = rm -rf

all: $(CONTENT)

img: $(IMAGES)

$(SITE)/%: %
	@echo ">> Copying $<"
	@$(MKDIR) $(@D)
	@$(CP) $< $(@D)

$(SITE)/%.html $(SITE)/%/index.html: md/%.md
	@echo ">> Converting $<"
	@$(MKDIR) $(@D)
	@$(PANDOC) -M page=$* -o $@ $<

img/%.png: svg/%.svg
	@echo ">> Converting $<"
	@$(MKDIR) $(@D)
	@$(INKSCAPE) $< | pngquant - > $@

serve:
	@python3 -m http.server --bind 127.0.0.1 --directory $(SITE)

clean:
	@$(RM) $(IMAGES) $(SITE)
