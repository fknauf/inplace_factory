#include "inplace_factory.hh"

#include <iostream>
#include <vector>

struct A {
  virtual void foo() const = 0;

  virtual ~A() { }
};

struct B : A{
  B(int i) : i_(i) { }
  B(B const &) = default;
  B(B&&) { std::cout << "B&&\n"; }
  virtual void foo() const { std::cout << "B " << i_ << "\n"; }
  ~B() { std::cout << "~B\n"; }
  int i_;
};

struct C : A {
  C() = default;
  C(C const &) = default;
  C(C&&) { std::cout << "C&&\n"; }

  virtual void foo() const { std::cout << "C\n"; }
  ~C() { std::cout << "~C\n"; }
};

struct D : A {
  virtual void foo() const { std::cout << "D\n"; }
  ~D() { std::cout << "~D\n"; }
};

int main() {
  inplace::factory<A, B, C> fct(
    [](inplace::factory<A, B, C> &fct, int n, int x) {
      if(n == 0) {
        fct.construct<B>(x);
      } else {
        fct.construct<C>();
      }
    },
    0, 1);

  if(fct) {
    fct.get().foo();
  }

  fct.construct<C>();
  if(fct) {
    fct.get().foo();
  }

  inplace::factory<A, B, C> fct2(fct);

  if(fct2) {
    fct2.get().foo();
  }

  fct2.construct<B>(2);
  fct = fct2;

  if(fct) fct.get().foo();

  {
    std::vector<inplace::factory<A, B, C>> v(10, fct);

    v[2].construct<C>();
    v[7] = v[2];
    v[6].construct<B>(10);
    v[3] = v[4] = v[6];

    for(auto &f : v) {
      if(f) f.get().foo();
    }
  }

  C c;

  fct.construct<C>(std::move(c));

  inplace::factory<A, B, C> fct3(std::move(fct));
  if(fct3) fct3.get().foo();

  // fct.construct<D>();
}