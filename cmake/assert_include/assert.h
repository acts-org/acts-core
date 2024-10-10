// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#if defined(NDEBUG)
#define _HAD_NDEBUG
#undef NDEBUG
#endif
#include_next <assert.h>
#if defined(_HAD_NDEBUG)
#undef _HAD_NDEBUG
#define NDEBUG
#endif
