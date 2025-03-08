#include "helper.h"

#include <stdio.h>
#include <stdlib.h>

void print_mono(Monome* mono, int sign);

Monome* make_mono(int coeff, int power) {
    Monome* mono = (Monome*)malloc(sizeof(Monome));
    if (mono == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    mono->coeff = coeff;
    mono->power = power;
    return mono;
}

Polynome* make_poly() {
    Polynome* poly = (Polynome*)calloc(1, sizeof(Polynome));
    if (poly == NULL) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return poly;
}

void add_to_poly(Polynome* poly, Monome* mono) {
    if (mono == NULL)
        return;

    Monome* m = poly->monos[mono->power];

    if (m == NULL)
        poly->monos[mono->power] = mono;
    else
        m->coeff += mono->coeff;
}

Polynome* add_polys(Polynome* poly1, Polynome* poly2) {
    for (int i = 0; i < POLY_SIZE; i++)
        add_to_poly(poly1, poly2->monos[i]);

    return poly1;
}

Polynome* sub_polys(Polynome* poly1, Polynome* poly2) {
    for (int i = 0; i < POLY_SIZE; i++) {
        if (poly2->monos[i] == NULL)
            continue;

        poly2->monos[i]->coeff *= -1;
        add_to_poly(poly1, poly2->monos[i]);
    }

    return poly1;
}

Polynome* multiply_polys(Polynome* poly1, Polynome* poly2) {
    Polynome* poly = make_poly();

    for (int i = 0; i < POLY_SIZE; i++) {
        Monome* mono1 = poly1->monos[i];

        if (mono1 == NULL)
            continue;

        for (int j = 0; j < POLY_SIZE; j++) {
            Monome* mono2 = poly2->monos[j];

            if (mono2 == NULL)
                continue;

            int coeff    = mono1->coeff * mono2->coeff;
            int power    = mono1->power + mono2->power;
            Monome* mono = make_mono(coeff, power);

            add_to_poly(poly, mono);
        }
    }

    return poly;
}

Polynome* dx_poly(Polynome* poly) {
    Polynome* dpoly = make_poly();

    for (int i = 0; i < POLY_SIZE; i++) {
        Monome* mono = poly->monos[i];

        if (mono == NULL || mono->power == 0)
            continue;

        int power     = mono->power;
        int coeff     = mono->coeff * power;
        Monome* dmono = make_mono(coeff, power - 1);
        add_to_poly(dpoly, dmono);
    }

    return dpoly;
}

void print_mono(Monome* mono, int sign) {
    if (mono->coeff == 0)
        return;

    if (sign && mono->coeff > 0)
        printf("+ ");
    else if (mono->coeff < 0)
        printf("- ");

    if (mono->coeff != 1) {
        printf("%d ", abs(mono->coeff));

        if (mono->power != 0)
            printf("* ");
    }

    if (mono->power != 0)
        printf("Y ");

    if (mono->power > 1)
        printf("^ %d ", mono->power);
}

void print_poly(Polynome* poly) {
    int not_first = 0;

    for (int i = POLY_SIZE - 1; i >= 0; i--) {
        if (poly->monos[i] == NULL)
            continue;

        print_mono(poly->monos[i], not_first);
        not_first = 1;
    }

    printf("\n");
}

int eval_poly(Polynome* poly, int x) {
    int n = 0;

    for (int i = POLY_SIZE - 1; i >= 0; i--) {
        Monome* mono = poly->monos[i];

        if (mono == NULL)
            continue;

        int p = 1;

        for (int j = 0; j < mono->power; j++)
            p *= x;

        int c = mono->coeff;
        n += c * p;
    }

    return n;
}
