#; -*-makefile-*-

EXEC += $~test $~print_position $~print_fen $~forced_mate

$~test: LDFLAGS += -lboost_unit_test_framework

$~test.o: $(call depend, $~test.cc)

$~test: $~test.o
	$(CXX) $& $(LDFLAGS) -o $@ $(LIBS)

$~print_position: $(call depend, $~print_position.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~forced_mate: $(call depend, $~forced_mate.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~print_fen: $(call depend, $~print_fen.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)
