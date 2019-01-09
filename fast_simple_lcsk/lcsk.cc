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

#include <algorithm>
#include <map>
#include <memory>
#include <queue>
#include <unordered_map>
#include <utility>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdlib>

#include "lcsk.h"
#include "match_events_queue.h"
#include "match_maker.h"
#include "match_pair.h"
using namespace std;

namespace {

vector<pair<int, int>> FillLcskReconstruction(
    const int k, std::shared_ptr<MatchPair> best) {
  std::vector<std::pair<int, int>> lcsk_recon;

  for (auto ft = best; ft != nullptr; ft = ft->prev) {
    int r = ft->end_row;
    int c = ft->end_col;

    if (ft->prev == nullptr ||
        (ft->prev->end_row + k <= ft->end_row &&
         ft->prev->end_col + k <= ft->end_col)) {
      for (int j = 0; j < k; ++j, --r, --c) {
        lcsk_recon.push_back(make_pair(r, c));
      }
    } else {
      assert(ft->prev->end_row + 1 == ft->end_row &&
             ft->prev->end_col + 1 == ft->end_col);
      lcsk_recon.push_back(make_pair(r, c));
    }
  }
  reverse(lcsk_recon.begin(), lcsk_recon.end());
  return lcsk_recon;
}

bool CompareByCol(const std::shared_ptr<MatchPair>& a,
                  const std::shared_ptr<MatchPair>& b) {
  return a->end_col < b->end_col;
}

void RowUpdate(
    const int k, const int row, MatchEventsQueue* events_ptr,
    vector<std::shared_ptr<MatchPair>>* compressed_table_ptr,
    vector<std::shared_ptr<MatchPair>>* prev_row_match_pairs,
    bool lcsk_plus) {
  auto& events = *events_ptr;
  auto& compressed_table = *compressed_table_ptr;
  auto& prev_row = *prev_row_match_pairs;

  std::tuple<int, int, std::shared_ptr<MatchPair>> event;

  vector<std::shared_ptr<MatchPair>> curr_row;
  int curr_continuation_index = 0;

  while (events.PopEnd(row, &event)) {
    int i = get<0>(event);
    int j = get<1>(event);
    assert(i == row);
    auto match_pair_end = get<2>(event);

    if (lcsk_plus) { // LCSk++
      while (curr_continuation_index < prev_row.size() &&
             prev_row[curr_continuation_index]->end_col + 1 < match_pair_end->end_col) {
        curr_continuation_index++;
      }

      if (curr_continuation_index < prev_row.size() &&
          prev_row[curr_continuation_index]->end_col + 1 == match_pair_end->end_col) {
        int continuation_dp = prev_row[curr_continuation_index]->dp + 1;
        if (continuation_dp > match_pair_end->dp) {
          match_pair_end->dp = continuation_dp;
          match_pair_end->prev = prev_row[curr_continuation_index];
        }
      }

      curr_row.emplace_back(match_pair_end);

      int dp = match_pair_end->dp;
      while (compressed_table.size() <= dp) {
        // fill with dummy values which will be overwritten in for loop below anyway.
        int idx = compressed_table.size();
        compressed_table.push_back(std::make_shared<MatchPair>(i+1, j+1, idx, nullptr));
      }

      for (int idx = dp; idx > dp - k && j < compressed_table[idx]->end_col; --idx) {
        compressed_table[idx] = match_pair_end;
      }
    } else { // LCSk
      int idx = match_pair_end->dp / k;
      if (idx == compressed_table.size()) {
        compressed_table.emplace_back(match_pair_end);
      } else if (j < compressed_table[idx]->end_col) {
        compressed_table[idx] = match_pair_end;
      }
    }
  }

  prev_row.swap(curr_row);
}

void AmortizedRowQuery(
    const int k, const int row, MatchEventsQueue* events_ptr,
    vector<std::shared_ptr<MatchPair>>* compressed_table_ptr) {
  auto& events = *events_ptr;
  auto& compressed_table = *compressed_table_ptr;

  int curr_threshold_index = 0;
  std::tuple<int, int, std::shared_ptr<MatchPair>> event;

  while (events.PopBegin(row, &event)) {
    int i = get<0>(event);
    int j = get<1>(event);
    assert(i == row);
    while (curr_threshold_index < compressed_table.size() &&
           compressed_table[curr_threshold_index]->end_col < j) {
      ++curr_threshold_index;
    }

    auto prev_best = compressed_table[curr_threshold_index - 1];
    auto match_pair = std::make_shared<MatchPair>(i + k - 1, j + k - 1, k, nullptr);
    if (prev_best->dp > 0) {
      match_pair->dp = prev_best->dp + k;
      match_pair->prev = prev_best;
    }
    events.AddEnd(make_tuple(i + k - 1, j + k - 1, match_pair));
  }
}

void ElementwiseRowQuery(
    const int k, const int row, MatchEventsQueue* events_ptr,
    vector<std::shared_ptr<MatchPair>>* compressed_table_ptr) {
  auto& events = *events_ptr;
  auto& compressed_table = *compressed_table_ptr;

  tuple<int, int, std::shared_ptr<MatchPair>> event;

  while (events.PopBegin(row, &event)) {
    int i = get<0>(event);
    int j = get<1>(event);
    assert(i == row);

    auto dummy_match_pair = std::make_shared<MatchPair>(0, j, 0, nullptr);
    auto prev_best =
      lower_bound(compressed_table.begin(), compressed_table.end(),
                  dummy_match_pair, CompareByCol) -
      1;
    // We reuse dummy_match_pair in order to keep the object counters precise
    // (otherwise the objects_created counter would roughly double due to
    // instantiation of the dummy object).
    //
    // The following several lines can be read as:
    // auto match_pair =
    //    std::make_shared<MatchPair>(i + k - 1, j + k - 1, k, nullptr);
    auto match_pair = dummy_match_pair;
    match_pair->end_row = i + k - 1;
    match_pair->end_col = j + k - 1;
    match_pair->dp = k;

    if ((*prev_best)->dp > 0) {
      match_pair->dp = (*prev_best)->dp + k;
      match_pair->prev = *prev_best;
    }
    events.AddEnd(make_tuple(i + k - 1, j + k - 1, match_pair));
  }
}

vector<pair<int, int>> LcskppSparseFastRealImpl(
    int k, int lcsk_plus, const vector<vector<int>> &matches) {
  MatchEventsQueue events;
  vector<std::shared_ptr<MatchPair>> compressed_table;
  compressed_table.emplace_back(std::make_shared<MatchPair>(-1, -1, 0, nullptr));
  // following invariants hold:
  //    LCSk++: compressed_table[i]->dp == i
  //    LCSk:   compressed_table[i]->dp == k*i
  vector<std::shared_ptr<MatchPair>> prev_row_match_pairs;

  for (int row = 0; row < matches.size(); ++row) {
    const vector<int> &row_matches = matches[row];
    for (int col : row_matches) {
      events.AddBegin(make_tuple(row, col, nullptr));
    }

    int table_row_size = compressed_table.size();
    int num_begin_events = row_matches.size();
    bool use_amortized_row_update = (table_row_size + num_begin_events <
                                     6 * num_begin_events * log(table_row_size) / log(2));

    if (use_amortized_row_update) {
      AmortizedRowQuery(k, row, &events, &compressed_table);
    } else {
      ElementwiseRowQuery(k, row, &events, &compressed_table);
    }

    RowUpdate(k, row, &events, &compressed_table, &prev_row_match_pairs, lcsk_plus);
  }

  auto best = compressed_table.back()->end_row != -1 ? compressed_table.back() : nullptr;
  return FillLcskReconstruction(k, best);
}

vector<pair<int, int>> LcskppSparseFastImpl(const std::string &a,
                                            const std::string &b,
                                            int k,
                                            int lcsk_plus,
                                            LcskppParams::Mode mode) {
  auto match_maker = MatchMaker::Create(a, b, k, PERFECT_HASH);
  vector<vector<int>> rows_matches;
  vector<pair<int, int>> matches;

  for (int row = 0; row <= a.size(); ++row) {
    vector<int> row_matches;
    match_maker->GetNextMatches(&row_matches);
    rows_matches.push_back(row_matches);
    for (int col : row_matches) {
      matches.emplace_back(row, col);
    }
  }

  switch (mode) {
    case LcskppParams::Mode::SINGLESTART: {
      return LcskppSparseFastRealImpl(k, lcsk_plus, rows_matches);
    }

    case LcskppParams::Mode::MULTISTART_2D_LOGARITHMIC: {
      vector<pair<int, int>> recon;
      while (matches.size()) {
        auto cm_matches = matches;
        sort(cm_matches.begin(), cm_matches.end(), [](pair<int, int> a, pair<int, int> b) {
            if (a.second != b.second) return a.second < b.second;
            return a.first < b.first;
        });
        while (cm_matches.size()) {
          vector<vector<int>> normalised_matches(a.size() + 1);
          for (auto match : cm_matches) {
            normalised_matches[match.first].push_back(match.second);
          }
          auto new_recon = LcskppSparseFastRealImpl(k, lcsk_plus, normalised_matches);
          recon.insert(recon.end(), new_recon.begin(), new_recon.end());
          vector<pair<int, int>> new_matches(cm_matches.begin() + (cm_matches.size() + 1) / 2,
                                             cm_matches.end());
          cm_matches = new_matches;
        }
        vector<pair<int, int>> new_matches(matches.begin() + (matches.size() + 1) / 2,
                                           matches.end());
        matches = new_matches;
      }
      sort(recon.begin(), recon.end());
      recon.erase(unique(recon.begin(), recon.end()), recon.end());
      return recon;
    }

    case LcskppParams::Mode::MULTISTART_2D_AGGRESSIVE: {
      assert(false);
      break;
    }
  }
  return {};
}

}  // namespace


// exposed functions

vector<pair<int, int>> LcskppSparseFast(
    const std::string &a, const std::string &b, const LcskppParams &params) {
  auto recon = LcskppSparseFastImpl(a, b, params.k, params.lcsk_plus, params.mode);
  if (params.reverse) {
    auto b_reversed = b;
    std::reverse(b_reversed.begin(), b_reversed.end());
    auto recon_reverse = LcskppSparseFastImpl(
        a, b_reversed, params.k, params.lcsk_plus, params.mode);
    int b_len = b.size();
    for (auto &match : recon_reverse) {
      match.second = b_len - 1 - match.second;
    }
    int middle = recon.size();
    recon.insert(recon.end(), recon_reverse.begin(), recon_reverse.end());
    inplace_merge(recon.begin(), recon.begin() + middle, recon.end());
  }
  return recon;
}
