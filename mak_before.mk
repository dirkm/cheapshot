# Called before including a sub directory makefile

# Save directory name
dstackp := $(dstackp).X
dstack$(dstackp) := $~

# Build directory name
~ := $~$(firstword $(dirlist))/

# Remove current directory from list
dirlist := $(call rest,$(dirlist))
