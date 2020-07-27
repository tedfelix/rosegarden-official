#pragma once

#include <string>

using namespace std;

int i4_min ( int i1, int i2 );
int levenshtein_distance ( int m, string s, int n, string t );
int *levenshtein_matrix ( int m, string s, int n, string t );
void timestamp ( );
