include makefile.inc

EXEC := 
cleanfiles :=
clean_default := *.o *.a *.so *.so.* *.d

default: progs

# Expand dependencies one level
dependless = %.o %.a %.d %.h
expand = $($(var)) $(var) $(var).d
depend_test = $(if $(filter $(dependless),$(var)),$(var),$(expand))
depend = $(sort $(foreach var,$(1),$(depend_test)))

include $(wildcard *.d)

& = $(filter-out %.h %.d,$^)

inc_before := mak_before.mk
inc_after := mak_after.mk 
rest = $(wordlist 2,$(words $(1)),$(1))
get_dirlist = $(inc_before) $~$(firstword $(1))/makefile.inc $(wildcard $~$(firstword $(1))/*.d) $(inc_after) $(if $(rest),$(call get_dirlist,$(rest)),)
scan_subdirs = $(eval dirlist:=$(SUBDIRS) $(dirlist)) $(call get_dirlist,$(SUBDIRS))

SUBDIRS := test

include $(scan_subdirs)

~:= tilde is broken in commands

cleanfiles += $(EXEC)

create_d = $(SHELL) -ec '$(CPP) -MM $(CPPFLAGS) $< | sed -n "H;$$ {g;s@.*:\(.*\)@$< := \$$\(wildcard\1\)\n$*.o $@: $$\($<\)@;p}" > $@'

%.cc.d: %.cc
	$(create_d)

progs: $(EXEC)

clean:
	-rm -f $(cleanfiles)

files:
	@echo $(EXEC)

makefile: makefile.inc

tests: test/test
	$^
