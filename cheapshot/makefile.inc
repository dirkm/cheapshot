#; -*-makefile-*-

SRC=$~io.cc

LIBCHEAPSHOT:=$~libcheapshot.a

$(LIBCHEAPSHOT): $(SRC:.cc=.o) $(call depend,$(SRC))
	ar rcs $@ $(filter-out %.cc,$&)
