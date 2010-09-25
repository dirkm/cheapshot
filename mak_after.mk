# Called after including a sub directory makefile

# Add defaults to clean files
cleanfiles += $(addprefix $~,$(clean_default))

# Restore directory name
~ := $(dstack$(dstackp))
dstackp := $(basename $(dstackp))
