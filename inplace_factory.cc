#include "inplace/factory.hh"

#include <iostream>
#include <vector>

struct A {
  virtual void foo() const = 0;
  virtual ~A() { }
};

struct B : virtual A {
  B(int i)          : i_(i)        { std::cout << "B(" << i << ")\n"; }
  B(B const &other) : i_(other.i_) { std::cout << "B(B const &)\n"; }
  B(B&&) = delete;

  virtual void foo() const { std::cout << "B::foo() mit i_ == " << i_ << "\n"; }
  ~B() { std::cout << "~B()\n"; }

private:
  int i_;
};

struct C : virtual A {
  C()          { std::cout << "C()\n"; }
  C(C const &) { std::cout << "C(C const &)\n"; }
  C(C&&)       { std::cout << "C(C&&)\n"; }
//  C(C&&) = delete;

  virtual void foo() const { std::cout << "C::foo()\n"; }
  ~C() { std::cout << "~C()\n"; }
};

struct D : virtual A {
  D() { std::cout << "D::D()\n"; }
  virtual void foo() const { std::cout << "D::foo()\n"; }
  ~D() { std::cout << "~D()\n"; }
};

struct E : B, C {
  E() : B(0), C() { std::cout << "E::E()\n"; }
  virtual void foo() const { std::cout << "E::foo()\n"; }
  ~E() { std::cout << "~E()\n"; }
};

struct M {
  M(int n) : n_(n) { }
  M(M const &) = delete;
  M(M&& other) {
    n_ = other.n_;
    other.n_ = 0;
  }

  int n_;
};

int main() {
  typedef inplace::factory<A, B, C, D, E> factory_t;

  std::cout <<
    "factory_t::has_copy_semantics == " << factory_t::has_copy_semantics << "\n"
    "factory_t::has_move_semantics == " << factory_t::has_move_semantics << "\n\n";

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

  factory_t fct6;
  fct6.construct<E>();
  fct6->foo();
  fct6.construct<E>();
  fct6->foo();

  factory_t fct7(fct6);
  fct7->foo();

  std::cout << "Tests mit move-, aber nicht kopierbarem Typ\n";

  inplace::factory<M, M> fct8;
  fct8.construct<M>(1);
  std::cout << "8: " << fct8->n_ << '\n';
  inplace::factory<M, M> fct9(std::move(fct8));
  if(fct8) std::cout << "8: " << fct8->n_ << '\n';
  if(fct9) std::cout << "9: " << fct9->n_ << '\n';
  fct8 = std::move(fct9);
  if(fct8) std::cout << "8: " << fct8->n_ << '\n';
  if(fct9) std::cout << "9: " << fct9->n_ << '\n';
  fct9.construct<M>(std::move(*fct8));
  if(fct8) std::cout << "8: " << fct8->n_ << '\n';
  if(fct9) std::cout << "9: " << fct9->n_ << '\n';

  std::cout << "\nAufräumarbeiten\n\n";
}
