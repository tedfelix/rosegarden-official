#pragma once

#include <string>

using namespace std;

int i4_min ( int i1, int i2 );
int levenshtein_distance ( int m, const string& s, int n, const string& t );
int *levenshtein_matrix ( int m, const string& s, int n, const string& t );
void timestamp ( );
