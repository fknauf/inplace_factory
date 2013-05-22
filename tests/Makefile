#!/usr/bin/make -f

TARGET = test

CPPFLAGS = -I..
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11

DEPFILE = .depend
SRCS    = test_main.cc group_plain.cc
OBJS    = $(SRCS:%.cc=%.o)
LDFLAGS = -lboost_unit_test_framework

.PHONY: all dep clean distclean

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
