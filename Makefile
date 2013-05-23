#!/usr/bin/make -f

TARGET = inplace_factory

CXX = clang++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++11

DEPFILE = .depend
SRCS = inplace_factory.cc
OBJS = $(SRCS:%.cc=%.o)

.PHONY: all dep clean distclean

all: dep $(TARGET)

$(DEPFILE): $(SRCS)
	$(CXX) $(CXXFLAGS) -MM $+ > $@

sinclude $(DEPFILE)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $+

clean:
	rm -f $(OBJS) $(TARGET)

distclean: clean
	rm -f $(DEPFILE) *~
