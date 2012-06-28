#include "inplace_factory.hh"

#include <iostream>
#include <vector>

struct A {
  virtual void foo() const = 0;
  virtual ~A() { }
};

struct B : A {
  B(int i)          : i_(i)        { std::cout << "B(" << i << ")\n"; }
  B(B const &other) : i_(other.i_) { std::cout << "B(B const &)\n"; }
  B(B&&) = delete;

  virtual void foo() const { std::cout << "B::foo() mit i_ == " << i_ << "\n"; }
  ~B() { std::cout << "~B()\n"; }

private:
  int i_;
};

struct C : A {
  C()          { std::cout << "C()\n"; }
  C(C const &) { std::cout << "C(C const &)\n"; }
  C(C&&)       { std::cout << "C(C&&)\n"; }

  virtual void foo() const { std::cout << "C::foo()\n"; }
  ~C() { std::cout << "~C()\n"; }
};

struct D : A {
  virtual void foo() const { std::cout << "D::foo()\n"; }
  ~D() { std::cout << "~D()\n"; }
};

int main() {
  typedef inplace::factory<A, B, C> factory_t;

  std::cout << "Konstruktion pv\n\n";
  factory_t fct;

  fct.construct<B>(1);
  fct->foo();
  fct.construct<C>();
  fct->foo();
  fct.construct<B>(2);
  fct->foo();

  std::cout << "\nKopiersemantik\n\n";

  factory_t fct2(fct);
  fct2->foo();
  fct2.construct<C>();
  fct = fct2;
  fct->foo();

  std::cout << "\nMovesemantik\n\n";

  factory_t fct3(std::move(fct));
  if(fct) {
    std::cout << "fct ist nicht leer.\n";
  } else if(!fct) {
    std::cout << "fct ist leer und ops void* und ! funktionieren.\n";
  }

  fct3->foo();
  fct2.construct<B>(3);
  fct3 = std::move(fct2);

  if(fct2) {
    std::cout << "fct2 ist nicht leer.\n";
  }

  fct3->foo();

  {
    std::cout << "\nVektorspaß\n\n";

    std::vector<factory_t> v(10, fct3);
    v[2].construct<C>();
    v[7] = v[2];
    v[6].construct<B>(10);
    v[3] = v[4] = v[6];

    for(auto &f : v) {
      f->foo();
    }
  }

  std::cout << "\nMove direkt\n\n";

  C c;
  factory_t fct4;
  fct4.construct<C>(std::move(c));
  fct4.construct<B>(B(4));
  fct4.construct<C>(C());

  std::cout << "\nInitialisierungsfunktion\n\n";

  factory_t fct5([](factory_t &f, int n, int x) {
      if(n == 0) {
        f.construct<B>(x);
      } else {
        f.construct<C>();
      }
    }, 0, 5);
  fct5->foo();

  std::cout << "\nAufräumarbeiten\n\n";
}
