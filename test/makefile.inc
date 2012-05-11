#; -*-makefile-*-

EXEC += $~unittest $~print_position $~print_fen $~forced_mate

$(EXEC): $(LIBCHEAPSHOT)

test: unittest ct_test tool_test

# object-files explicitely generated, to view the assembly
$~unittest_bitops.o: $(call depend, $~unittest_bitops.cc)
$~unittest_other.o: $(call depend, $~unittest_other.cc)
$~unittest_io.o: $(call depend, $~unittest_io.cc)
$~unittest_ab.o: $(call depend, $~unittest_ab.cc)

$~unittest: LDFLAGS += -lboost_unit_test_framework

$~unittest: $~unittest_bitops.o $~unittest_other.o $~unittest_io.o $~unittest_ab.o
	$(CXX) $& $(LDFLAGS) -o $@ $(LIBS)

$~print_position: $(call depend, $~print_position.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~forced_mate: $(call depend, $~forced_mate.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

$~print_fen: $(call depend, $~print_fen.cc)
	$(CXX) $& $(CPPFLAGS) $(LDFLAGS) -o $@ $(LIBS)

.PHONY: test unittest ct_test test_report

unittest: $~unittest
	$^ --catch_system_errors=0
	# $^ --log_level=all --catch_system_errors=0

ct_test: $(call depend, $~ct_test.cc) $~ct_test.o

tool_test: $~forced_mate
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
