// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include <functional>

#include "fast_simple_lcsk/lcsk.h"
#include "util/lcsk_testing.h"
#include "util/random_strings.h"
using namespace std;

const int only_run_fast_version = 0;

// Length of the strings.
const int kStringLen = 100;

// Number of performed simulations.
const int kSimulationRuns = 10000;

// Default value of the k parameter.
const int kK = 3;

// If kPerr is set to -1 then rand-to-rand strings are aligned, otherwise
// rand-to-modified-copy simulations are performed.
// const double kPerr = -1.0;
const double kPerr = 0.1;

int test_lcsk(const string &a, const string &b,
    LcskppParams params,
    function<vector<pair<int, int>>(string, string)> fake,
    function<bool(string a, string b, vector<pair<int, int>>)> checker) {
  vector<pair<int, int>> slow_recon;
  if (!only_run_fast_version) {
    slow_recon = fake(a, b);
  }

  auto fast_recon = LcskppSparseFast(a, b, params);

  if (!only_run_fast_version) {
    assert(slow_recon.size() == fast_recon.size());
    assert(checker(a, b, slow_recon));
  }
  assert(checker(a, b, fast_recon));

  return fast_recon.size();
}

map<int, double> run_simulations(
    function<pair<string, string>(void)> generate_strings,
    LcskppParams params,
    function<vector<pair<int, int>>(string, string)> fake,
    function<bool(string a, string b, vector<pair<int, int>>)> checker) {
  map<int, double> distr;
  for (int i = 0; i < kSimulationRuns; ++i) {
    pair<string, string> ab = generate_strings();
    const string &a = ab.first;
    const string &b = ab.second;
    auto res = test_lcsk(a, b, params, fake, checker);
    distr[res] += 1.0 / kSimulationRuns;
  }
  return distr;
}

void LcskTest() {
  printf("LcskTest\n");
  printf("Running tests on %d random pairs ", kSimulationRuns);
  printf("with the following parameters:\n");
  printf("  string length=%d\n", kStringLen);
  printf("  k=%d\n", kK);
  printf("  pErr=%0.2lf\n", kPerr);
  printf("  lcsk_plus=false\n");

  LcskppParams params(kK);
  params.lcsk_plus = false;
  auto distr = run_simulations([] () {
        auto a = generate_string(kStringLen);
        auto b = generate_similar(a, kPerr);
        return pair<string, string>(a, b);
      },
      params,
      [] (const string &a, const string &b) {
        return LcskSparseSlow(a, b, kK);
      },
      [] (const string &a, const string &b, const vector<pair<int, int>> &recon) {
        return ValidLcsk(a, b, kK, recon);
      });

  double sum_prob = 0;
  double e_lcs = 0;

  for (int i = 0; i <= kStringLen; ++i) {
    double p = distr[i];
    sum_prob += p;
    e_lcs += p * i;
  }

  assert(0.99999 <= sum_prob <= 1.00001);
  printf("Expected LCSk++=%0.3lf\n", e_lcs);
  printf("Test PASSED!\n");
}

void LcskppTest() {
  printf("LcskppTest\n");
  printf("Running tests on %d random pairs ", kSimulationRuns);
  printf("with the following parameters:\n");
  printf("  string length=%d\n", kStringLen);
  printf("  k=%d\n", kK);
  printf("  pErr=%0.2lf\n", kPerr);

  auto distr = run_simulations([] () {
        auto a = generate_string(kStringLen);
        auto b = generate_similar(a, kPerr);
        return pair<string, string>(a, b);
      },
      LcskppParams(kK),
      [] (const string &a, const string &b) {
        return LcskppSparseSlow(a, b, kK);
      },
      [] (const string &a, const string &b, const vector<pair<int, int>> &recon) {
        return ValidLcskpp(a, b, kK, recon);
      });

  double sum_prob = 0;
  double e_lcs = 0;

  for (int i = 0; i <= kStringLen; ++i) {
    double p = distr[i];
    sum_prob += p;
    e_lcs += p * i;
  }

  assert(0.99999 <= sum_prob <= 1.00001);
  printf("Expected LCSk++=%0.3lf\n", e_lcs);
  printf("Test PASSED!\n");
}

void LcskppReverseTest() {
  printf("LcskppReverseTest\n");
  LcskppParams params(kK);
  params.reverse = true;
  auto recon = LcskppSparseFast("actgXxxCCCTTxxxXxtaacctxXxxGGAAz",
                                "yyyactgYYyAAGGyytaacctYyyTTCCCz",
                                params);
  decltype(recon) expected =
      {{0, 3}, {1, 4}, {2, 5}, {3, 6},
       {7, 29}, {8, 28}, {9, 27}, {10, 26}, {11, 25},
       {17, 16}, {18, 17}, {19, 18}, {20, 19}, {21, 20}, {22, 21},
       {27, 13}, {28, 12}, {29, 11}, {30, 10}};
  assert(recon == expected);
  printf("Test PASSED!\n");
}

void LcskppMultistartTest() {
  printf("LcskppMultistartTest\n");
  LcskppParams params(kK);
  params.mode = LcskppParams::Mode::MULTISTART_2D_LOGARITHMIC;
  auto recon = LcskppSparseFast(
      "AAAbbbBBBcccAAAdddCCCeeeBBBfffAAA", "AAAbbbBBBcccAAAdddCCC", params);
  decltype(recon) expected = {{0, 0}, {1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5},
                              {6, 6}, {7, 7}, {8, 8}, {9, 9}, {10, 10},
                              {11, 11}, {12, 12}, {13, 13}, {14, 14}, {15, 15},
                              {16, 16}, {17, 17}, {18, 18}, {19, 19}, {20, 20},
                              {24, 6}, {25, 7}, {26, 8}, {30, 12}, {31, 13},
                              {32, 14}};
  assert(recon == expected);
  printf("Test PASSED!\n");
}

void LcskppMultistartAggressiveTest() {
  printf("LcskppMultistartAggressiveTest\n");
  LcskppParams params(kK);
  params.mode = LcskppParams::Mode::MULTISTART_AGGRESSIVE;
  params.aggressive_runs = 3;
  auto recon = LcskppSparseFast(
      "AABBxCCCxxDDDxxEEFFxFFGGFBBB", "AABByDDDyyCCCyyFFGGFEEFFyBBByyAABB", params);
  decltype(recon) expected = {
    {0, 0}, {0, 30}, {1, 1}, {1, 31}, {2, 2}, {2, 32}, {3, 3}, {3, 33},
    {5, 10}, {6, 11}, {7, 12},
    {10, 5}, {11, 6}, {12, 7},
    {15, 20}, {16, 21}, {17, 22}, {18, 23},
    {20, 15}, {21, 16}, {22, 17}, {23, 18}, {24, 19},
    {25, 25}, {26, 26}, {27, 27}};
  assert(recon == expected);
  printf("Test PASSED!\n");
}

int main(int argc, char *argv[]) {
  srand(1603);
  LcskTest();
  LcskppTest();
  LcskppReverseTest();
  LcskppMultistartTest();
  LcskppMultistartAggressiveTest();
  return 0;
}
