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

#ifndef LCSK
#define LCSK

#include <string>
#include <utility>
#include <vector>

struct LcskppParams {
  LcskppParams() = default;
  LcskppParams(int k) : k(k) {}

  enum class Mode {
    // Single lcsk is run on strings.
    SINGLESTART,
    // Multiple lcsks are run.
    // TODO: write description of multistart modes.
    MULTISTART_2D_LOGARITHMIC,
    MULTISTART_AGGRESSIVE,
  };

  // If true lcsk++ is used, otherwise standard lcsk algorithm.
  bool lcsk_plus = true;
  // If true matching is also calculated on reversed string.
  bool reverse = false;
  Mode mode = Mode::SINGLESTART;
  // Minimal length of match to considered.
  int k = 3;
  // Number of runs in MULTISTART_AGGRESSIVE mode, in other modes ignored.
  int aggressive_runs = 3;
};

// Find LCSk of strings a and b.
std::vector<std::pair<int, int>> LcskppSparseFast(
    const std::string &a, const std::string &b, const LcskppParams &params);

#endif
