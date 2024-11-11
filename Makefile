BASE_URL =
BASE_PATH =

SITE = _site

PAGES = $(filter-out index, $(basename $(notdir $(wildcard md/*.md))))
CONTENT = $(addprefix $(SITE)/, etc css img index.html $(addsuffix /index.html, $(PAGES)))

PANDOC = pandoc -d defaults.yml -M base_url=$(BASE_URL) -M base_path=$(BASE_PATH)

MKDIR = mkdir -p
CP = cp -r
RM = rm -rf

all: $(CONTENT)

$(SITE)/%: %
	@echo ">> Copying $^"
	@$(MKDIR) $(@D)
	@$(CP) $^ $@

$(SITE)/%.html $(SITE)/%/index.html: md/%.md
	@echo ">> Converting $^"
	@$(MKDIR) $(@D)
	@$(PANDOC) -M page=$* -o $@ $^

serve:
	@python3 -m http.server --bind 127.0.0.1 --directory $(SITE)

clean:
	@$(RM) $(SITE)
