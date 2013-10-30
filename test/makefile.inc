#; -*-makefile-*-

EXEC += $~unittest $~print_position $~print_fen $~forced_mate $~parse_pgn

$(EXEC): $(LIBCHEAPSHOT)

test: unittest ct_test tool_test

UNITTEST_SRC:=$~unittest_bitops.cc $~unittest_other.cc $~unittest_io.cc $~unittest_ab.cc $~unittest_compress.cc

$~unittest: $(call depend, $(UNITTEST_SRC))

$~unittest: LDFLAGS += -lboost_unit_test_framework

$~unittest: $(call depend, $(UNITTEST_SRC)) $(UNITTEST_SRC:.cc=.o)
	$(CXX) $(filter-out %.cc,$&) $(LDFLAGS) -o $@ $(LIBS)

$~print_position: $(call depend, $~print_position.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~forced_mate: $(call depend, $~forced_mate.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~print_fen: $(call depend, $~print_fen.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~parse_pgn: $(call depend, $~parse_pgn.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

.PHONY: test unittest currenttest ct_test test_report

unittest: $~unittest
	$^ --catch_system_errors=0

currenttest: $~unittest
	$^ --run_test=basic_engine_suite/rook_queen_test --log_level=all --catch_system_errors=0

# $^ --run_test=basic_engine_suite/rook_queen_test --log_level=all --catch_system_errors=0
# $^ --catch_system_errors=0
# $^ --log_level=all --catch_system_errors=0

ct_test: $(call depend, $~ct_test.cc) $~ct_test.o

tool_test: $(EXEC)
	test/tool_test.sh

test_report: $~unittest
	@echo -n "Date: "
	@date
	@echo "Compiler info:"
	@$(CXX) -v 2>&1
	@echo "CFlags: $(CFLAGS)"
	@echo "Cpus: "
	@grep "model name" /proc/cpuinfo
	@echo "Last git commit: "
	@git --no-pager log --max-count=1
	@echo "Tests"
	@ $^
