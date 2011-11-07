#; -*-makefile-*-

EXEC += $~test $~print_canvas

$~test: LDFLAGS += -lboost_unit_test_framework

$~test.o: $(call depend, $~test.cc)

$~test: $~test.o
	$(CXX) $& $(LDFLAGS) -o $@ $(LIBS)

$~print_canvas: $(call depend, $~print_canvas.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)
