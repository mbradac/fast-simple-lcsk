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

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cassert>

#include "fast_simple_lcsk/lcsk.h"
#include "fast_simple_lcsk/match_pair.h"

using namespace std;

const string kModeMa = "MA";
const string kModeLcskpp = "LCSKPP";

void print_usage_and_exit() {
  printf(
    "Compute LCSk++ of two plain texts.\n\n"
    "Usage: ./main k input1 input2 output [--reverse] [--mode MODE] [--runs RUNS]\n"
    "If --reverse flag is used lcsk is run on both normal and reversed string"
    "Mode can be either LCSKPP (default), MS (multistart_2dlogarithmic) "
    "or MSA (multistart_aggressive)\n"
    "In MSA mode you can specify number of runs with --runs flag. In other modes "
    "that flag is ignored."
    "Unlike most unix programs optional flags should be after mandatory args\n\n"
    "Example: ./main 4 test/tests/test.1.A test/tests/test.1.B out\n"
    "finds LCSK++ of files `test/tests/test.1.A` and `test/tests/test.1.B`\n"
    "and writes it to `output`\n"
  );
  exit(0);
}

int main(int argc, char** argv) {
  if (argc < 5) {
    print_usage_and_exit();
  };

  int k = stoi(argv[1]);
  ifstream infile1(argv[2]);
  string A;
  getline(infile1, A);

  ifstream infile2(argv[3]);
  string B;
  getline(infile2, B);

  printf("Sequence 1 length: %d\n", (int)A.size());
  printf("Sequence 2 length: %d\n", (int)B.size());

  LcskppParams params(k);
  {
    int i = 5;
    while (i < argc) {
      if (string(argv[i]) == "--reverse") {
        params.reverse = true;
      } else if (string(argv[i]) == "--mode") {
        if (i + 1 == argc) {
          print_usage_and_exit();
        }
        string mode = argv[++i];
        if (mode == "LCSKPP") {
          params.mode = LcskppParams::Mode::SINGLESTART;
        } else if (mode == "MS") {
          params.mode = LcskppParams::Mode::MULTISTART_2D_LOGARITHMIC;
        } else if (mode == "MSA") {
          params.mode = LcskppParams::Mode::MULTISTART_AGGRESSIVE;
        } else {
          print_usage_and_exit();
        }
      } else if (string(argv[i]) == "--runs") {
        if (i + 1 == argc) {
          print_usage_and_exit();
        }
        params.aggressive_runs = stoi(argv[++i]);
      } else {
        print_usage_and_exit();
      }
      ++i;
    }
  }

  printf("Computing LCSk++..\n");
  auto recon = LcskppSparseFast(A, B, params);
  int length = recon.size();

  printf("LCSk++ length: %d\n", length);
  cout << "MatchPairs created: " << ObjectCounter<MatchPair>::objects_created << endl;
  cout << "Max Alive MatchPairs: " << ObjectCounter<MatchPair>::max_objects_alive << endl;

  auto r = freopen(argv[4], "w", stdout);
  int last_position = -1;
  for (auto& p: recon) {
    if (last_position != p.first) {
      putchar(A[p.first]);
      last_position = p.first;
    }
  }
  return 0;
}
