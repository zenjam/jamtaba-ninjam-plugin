ROOT_PATH = "$$PWD/.."
TRANSLATIONS_DIR = "$$ROOT_PATH/translations/"

CODECFORSRC = UTF-8

LANGUAGES = en ja_JP pt_BR es_CL fr_FR de_DE pl_PL da_DK it_IT ko_KR uk_UA

# parameters: var, prepend, append
defineReplace(prependAll) {
  for(a, $$1): result += $$2$${a}$$3
  return($$result)
}

# parameters: var
defineReplace(extractLanguageAll) {
  for(a, $$1) {
    !equals(a, "en") {
      equals(a, "zh_CN") | equals(a, "zh_CN") {
        result += $$a
      } else {
        parts = $$split(a, "_")
        result += $$first(parts)
      }
    }
  }
  return($$result)
}

# Embed Qt translations
LCONVERT_PATTERNS = qtbase qtmultimedia
LCONVERT_LANGS = $$extractLanguageAll(LANGUAGES)
include(lconvert.pri)

# Embed custom translations
TRANSLATIONS = $$prependAll(LANGUAGES, $$TRANSLATIONS_DIR, .ts)
QM_FILES_RESOURCE_PREFIX=tr
CONFIG += lrelease embed_translations
