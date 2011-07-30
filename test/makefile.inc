#; -*-makefile-*-

EXEC += $~test $~print_canvas

$~test: LDFLAGS += -lboost_unit_test_framework

$~test: $(call depend, $~test.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~print_canvas: $(call depend, $~print_canvas.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)
