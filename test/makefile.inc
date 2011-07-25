#; -*-makefile-*-

EXEC += $~test $~print_layout

$~test: LDFLAGS += -lboost_unit_test_framework

$~test: $(call depend, $~test.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~print_layout: $(call depend, $~print_layout.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)
