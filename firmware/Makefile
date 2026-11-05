# Run the firmware logic tests on your PC (no ESP32 needed): make test
CXX ?= g++
OUT ?= build
.PHONY: test
test:
	mkdir -p $(OUT)
	$(CXX) -std=c++11 -Wall -Wextra -Werror -Ilib/core test/test_core.cpp lib/core/Fsm.cpp lib/core/Json.cpp -o $(OUT)/test_core
	$(OUT)/test_core
