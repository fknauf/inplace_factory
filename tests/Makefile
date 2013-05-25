#!/usr/bin/make -f

TARGET   = test

CXX      = g++-4.8
#CXX      = clang++

CPPFLAGS = -I..
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11 -g -O0

SRCS     =               \
  test_main.cc           \
  group_exceptions.cc    \
  group_mixed.cc         \
  group_multi.cc         \
  group_nocopy.cc        \
  group_nocopy_nomove.cc \
  group_nomove.cc        \
  group_plain.cc         \
  group_references.cc

DEPFILE  = .depend
OBJS     = $(SRCS:%.cc=%.o)
LDFLAGS  = -lboost_unit_test_framework

.PHONY: all dep clean distclean check

all: dep $(TARGET)

$(DEPFILE): $(SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $+ > $@

sinclude $(DEPFILE)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(DEPFILE) *~

check: $(TARGET)
	valgrind ./$(TARGET)
