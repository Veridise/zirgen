// Copyright 2024 RISC Zero, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This code is automatically generated

use risc0_zkp::taps::{TapData, TapSet};

#[allow(missing_docs)]

pub const TAPSET: &TapSet = &TapSet::<'static> {
    taps: &[
        TapData {
            offset: 0,
            back: 0,
            group: 0,
            combo: 0,
            skip: 1,
        },
        TapData {
            offset: 0,
            back: 0,
            group: 1,
            combo: 0,
            skip: 1,
        },
        TapData {
            offset: 1,
            back: 0,
            group: 1,
            combo: 0,
            skip: 1,
        },
        TapData {
            offset: 2,
            back: 0,
            group: 1,
            combo: 0,
            skip: 1,
        },
        TapData {
            offset: 0,
            back: 0,
            group: 2,
            combo: 1,
            skip: 3,
        },
        TapData {
            offset: 0,
            back: 1,
            group: 2,
            combo: 1,
            skip: 3,
        },
        TapData {
            offset: 0,
            back: 2,
            group: 2,
            combo: 1,
            skip: 3,
        },
    ],
    combo_taps: &[0, 0, 1, 2],
    combo_begin: &[0, 1, 4],
    group_begin: &[0, 1, 4, 7],
    combos_count: 2,
    reg_count: 5,
    tot_combo_backs: 4,
    // TODO: Generate these instead of hardcoding:
    group_names: &["accum", "code", "data"],
};