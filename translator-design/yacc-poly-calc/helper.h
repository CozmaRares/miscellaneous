#ifndef HELPER_H
#define HELPER_H

typedef struct monome_t {
    int coeff;
    int power;
} Monome;

#define POLY_SIZE 100

typedef struct polynome_t {
    Monome* monos[POLY_SIZE];
} Polynome;

Monome* make_mono(int coeff, int power);

Polynome* make_poly();
void add_to_poly(Polynome* poly, Monome* mono);

Polynome* add_polys(Polynome* poly1, Polynome* poly2);
Polynome* sub_polys(Polynome* poly1, Polynome* poly2);
Polynome* multiply_polys(Polynome* poly1, Polynome* poly2);
Polynome* dx_poly(Polynome* poly);

int eval_poly(Polynome* poly, int x);

void print_poly(Polynome* poly);

#endif  // HELPER_H
