// Copyright 2022 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "isp.h"
#include "osd.h"

#include "rockiva.h"
#include "rtsp_demo.h"

#include "rk_mpi_mmz.h"
#include <rk_debug.h>
#include <rk_mpi_ivs.h>
#include <rk_mpi_mb.h>
#include <rk_mpi_rgn.h>
#include <rk_mpi_sys.h>
#include <rk_mpi_venc.h>
#include <rk_mpi_vi.h>
#include <rk_mpi_vpss.h>

#include <inttypes.h> // PRId64

#include "rk_comm_tde.h"
#include "rk_mpi_tde.h"
#include <rga/im2d.h>
#include <rga/rga.h>

int rk_video_init();
int rk_video_deinit();
int rk_video_restart();
int rk_take_photo();
