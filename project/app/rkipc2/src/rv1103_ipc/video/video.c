// Copyright 2022 Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "video.h"
#include "audio.h"
#include "rockiva.h"
#include <stdio.h>

#define HAS_VO 0
#if HAS_VO
#include <rk_mpi_vo.h>
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "video.c"

#define RKISP_MAINPATH 0
#define RKISP_SELFPATH 1
#define RKISP_FBCPATH 2
#define VIDEO_PIPE_0 0
#define VIDEO_PIPE_1 1
#define VIDEO_PIPE_2 2
#define VIDEO_PIPE_3 3
#define JPEG_VENC_CHN 4
#define DRAW_NN_VENC_CHN_ID 0
#define VPSS_ROTATE 6
#define VPSS_BGR 0
#define DRAW_NN_OSD_ID 7
#define RED_COLOR 0x0000FF
#define BLUE_COLOR 0xFF0000

#define RTSP_URL_0 "/live/0"
#define RTSP_URL_1 "/live/1"
#define RTSP_URL_2 "/live/2"
#define RTMP_URL_0 "rtmp://127.0.0.1:1935/live/mainstream"
#define RTMP_URL_1 "rtmp://127.0.0.1:1935/live/substream"
#define RTMP_URL_2 "rtmp://127.0.0.1:1935/live/thirdstream"

pthread_mutex_t g_rtsp_mutex = PTHREAD_MUTEX_INITIALIZER;
rtsp_demo_handle g_rtsplive = NULL;
rtsp_session_handle g_rtsp_session_0, g_rtsp_session_1, g_rtsp_session_2;
static int get_jpeg_cnt = 0;
static int enable_ivs, enable_jpeg, enable_venc_0, enable_venc_1, enable_rtsp;
static int g_vi_chn_id, enable_npu, enable_wrap, enable_osd;
static int g_video_run_ = 1;
static int pipe_id_ = 0;
static int dev_id_ = 0;
static int g_nn_osd_run_ = 1;
static int cycle_snapshot_flag = 0;
static const char *tmp_output_data_type = "H.264";
static const char *tmp_rc_mode;
static const char *tmp_h264_profile;
static const char *tmp_smart;
static const char *tmp_gop_mode;
static const char *tmp_rc_quality;
static pthread_t venc_thread_0, venc_thread_1, jpeg_venc_thread_id,
    cycle_snapshot_thread_id, get_nn_update_osd_thread_id, get_vi_2_send_thread,
    get_ivs_result_thread;

static MPP_CHN_S vi_chn, venc_chn, ivs_chn;

typedef enum rkCOLOR_INDEX_E {
	RGN_COLOR_LUT_INDEX_0 = 0,
	RGN_COLOR_LUT_INDEX_1 = 1,
} COLOR_INDEX_E;

static void *rkipc_get_venc_0(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcVenc0", 0, 0, 0);
	VENC_STREAM_S stFrame;
	VI_CHN_STATUS_S stChnStatus;
	int loopCount = 0;
	int ret = 0;
	// FILE *fp = fopen("/data/venc.h265", "wb");
	stFrame.pstPack = malloc(sizeof(VENC_PACK_S));

	while (g_video_run_) {
		// 5.get the frame
		ret = RK_MPI_VENC_GetStream(VIDEO_PIPE_0, &stFrame, 1000);
		if (ret == RK_SUCCESS) {
			void *data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
			// fwrite(data, 1, stFrame.pstPack->u32Len, fp);
			// fflush(fp);
			// LOG_DEBUG("Count:%d, Len:%d, PTS is %" PRId64", enH264EType is %d\n", loopCount,
			// stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS,
			// stFrame.pstPack->DataType.enH264EType);

			if (g_rtsplive && g_rtsp_session_0) {
				pthread_mutex_lock(&g_rtsp_mutex);
				rtsp_tx_video(g_rtsp_session_0, data, stFrame.pstPack->u32Len,
				              stFrame.pstPack->u64PTS);
				rtsp_do_event(g_rtsplive);
				pthread_mutex_unlock(&g_rtsp_mutex);
			}
			if ((stFrame.pstPack->DataType.enH264EType == H264E_NALU_IDRSLICE) ||
			    (stFrame.pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ||
			    (stFrame.pstPack->DataType.enH265EType == H265E_NALU_IDRSLICE) ||
			    (stFrame.pstPack->DataType.enH265EType == H265E_NALU_ISLICE)) {
				rk_storage_write_video_frame(0, data, stFrame.pstPack->u32Len,
				                             stFrame.pstPack->u64PTS, 1);
			} else {
				rk_storage_write_video_frame(0, data, stFrame.pstPack->u32Len,
				                             stFrame.pstPack->u64PTS, 0);
			}
			// 7.release the frame
			ret = RK_MPI_VENC_ReleaseStream(VIDEO_PIPE_0, &stFrame);
			if (ret != RK_SUCCESS) {
				LOG_ERROR("RK_MPI_VENC_ReleaseStream fail %x\n", ret);
			}
			loopCount++;
		} else {
			LOG_ERROR("RK_MPI_VENC_GetStream timeout %x\n", ret);
		}
	}
	if (stFrame.pstPack)
		free(stFrame.pstPack);
	// if (fp)
	// fclose(fp);

	return 0;
}

static void *rkipc_get_venc_1(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcVenc1", 0, 0, 0);
	VENC_STREAM_S stFrame;
	VI_CHN_STATUS_S stChnStatus;
	int loopCount = 0;
	int ret = 0;
	stFrame.pstPack = malloc(sizeof(VENC_PACK_S));

	while (g_video_run_) {
		// 5.get the frame
		ret = RK_MPI_VENC_GetStream(VIDEO_PIPE_1, &stFrame, 1000);
		if (ret == RK_SUCCESS) {
			void *data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
			// LOG_INFO("Count:%d, Len:%d, PTS is %" PRId64", enH264EType is %d\n", loopCount,
			// stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS,
			// stFrame.pstPack->DataType.enH264EType);
			if (g_rtsplive && g_rtsp_session_1) {
				pthread_mutex_lock(&g_rtsp_mutex);
				rtsp_tx_video(g_rtsp_session_1, data, stFrame.pstPack->u32Len,
				              stFrame.pstPack->u64PTS);
				rtsp_do_event(g_rtsplive);
				pthread_mutex_unlock(&g_rtsp_mutex);
			}
			if ((stFrame.pstPack->DataType.enH264EType == H264E_NALU_IDRSLICE) ||
			    (stFrame.pstPack->DataType.enH264EType == H264E_NALU_ISLICE) ||
			    (stFrame.pstPack->DataType.enH265EType == H265E_NALU_IDRSLICE) ||
			    (stFrame.pstPack->DataType.enH265EType == H265E_NALU_ISLICE)) {
				rk_storage_write_video_frame(1, data, stFrame.pstPack->u32Len,
				                             stFrame.pstPack->u64PTS, 1);
			} else {
				rk_storage_write_video_frame(1, data, stFrame.pstPack->u32Len,
				                             stFrame.pstPack->u64PTS, 0);
			}
			// 7.release the frame
			ret = RK_MPI_VENC_ReleaseStream(VIDEO_PIPE_1, &stFrame);
			if (ret != RK_SUCCESS)
				LOG_ERROR("RK_MPI_VENC_ReleaseStream fail %x\n", ret);
			loopCount++;
		} else {
			LOG_ERROR("RK_MPI_VENC_GetStream timeout %x\n", ret);
		}
	}
	if (stFrame.pstPack)
		free(stFrame.pstPack);

	return 0;
}

static void *rkipc_get_jpeg(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcGetJpeg", 0, 0, 0);
	VENC_STREAM_S stFrame;
	VI_CHN_STATUS_S stChnStatus;
	int loopCount = 0;
	int ret = 0;
	char file_name[128] = {0};
	char record_path[256];

	memset(&record_path, 0, sizeof(record_path));
	strcat(record_path, rk_param_get_string("storage:mount_path", "/userdata"));
	strcat(record_path, "/");
	strcat(record_path, rk_param_get_string("storage.0:folder_name", "video0"));

	stFrame.pstPack = malloc(sizeof(VENC_PACK_S));
	// drop first frame

	ret = RK_MPI_VENC_GetStream(JPEG_VENC_CHN, &stFrame, 1000);
	if (ret == RK_SUCCESS)
		RK_MPI_VENC_ReleaseStream(JPEG_VENC_CHN, &stFrame);
	else
		LOG_ERROR("RK_MPI_VENC_GetStream timeout %x\n", ret);
	while (g_video_run_) {
		if (!get_jpeg_cnt) {
			usleep(300 * 1000);
			continue;
		}
		// 5.get the frame
		ret = RK_MPI_VENC_GetStream(JPEG_VENC_CHN, &stFrame, 1000);
		if (ret == RK_SUCCESS) {
			void *data = RK_MPI_MB_Handle2VirAddr(stFrame.pstPack->pMbBlk);
			LOG_DEBUG("Count:%d, Len:%d, PTS is %" PRId64 ", enH264EType is %d\n", loopCount,
			          stFrame.pstPack->u32Len, stFrame.pstPack->u64PTS,
			          stFrame.pstPack->DataType.enH264EType);
			// save jpeg file
			time_t t = time(NULL);
			struct tm tm = *localtime(&t);
			snprintf(file_name, 128, "%s/%d%02d%02d%02d%02d%02d.jpeg", record_path,
			         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min,
			         tm.tm_sec);
			LOG_DEBUG("file_name is %s, u32Len is %d\n", file_name, stFrame.pstPack->u32Len);
			FILE *fp = fopen(file_name, "wb");
			if (fp == NULL) {
				LOG_ERROR("fp is NULL\n");
			} else {
				fwrite(data, 1, stFrame.pstPack->u32Len, fp);
			}
			// 7.release the frame
			ret = RK_MPI_VENC_ReleaseStream(JPEG_VENC_CHN, &stFrame);
			if (ret != RK_SUCCESS) {
				LOG_ERROR("RK_MPI_VENC_ReleaseStream fail %x\n", ret);
			}
			if (fp) {
				fflush(fp);
				fclose(fp);
			}
			loopCount++;
		} else {
			LOG_ERROR("RK_MPI_VENC_GetStream timeout %x\n", ret);
		}
		get_jpeg_cnt--;
		RK_MPI_VENC_StopRecvFrame(JPEG_VENC_CHN);
		// usleep(33 * 1000);
	}
	if (stFrame.pstPack)
		free(stFrame.pstPack);

	return 0;
}

int rk_take_photo() {
	LOG_DEBUG("start\n");
	if (get_jpeg_cnt) {
		LOG_WARN("the last photo was not completed\n");
		return -1;
	}
	if (rkipc_storage_dev_mount_status_get() != DISK_MOUNTED) {
		LOG_WARN("dev not mount\n");
		return -1;
	}
	VENC_RECV_PIC_PARAM_S stRecvParam;
	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = 1;
	RK_MPI_VENC_StartRecvFrame(JPEG_VENC_CHN, &stRecvParam);
	get_jpeg_cnt++;

	return 0;
}

static void *rkipc_cycle_snapshot(void *arg) {
	LOG_INFO("start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcCycleSnapshot", 0, 0, 0);

	while (g_video_run_ && cycle_snapshot_flag) {
		usleep(rk_param_get_int("video.jpeg:snapshot_interval_ms", 1000) * 1000);
		rk_take_photo();
	}
	LOG_INFO("exit %s thread, arg:%p\n", __func__, arg);

	return 0;
}

static void *rkipc_get_vi_2_send(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcGetVi2", 0, 0, 0);
	int ret;
	int32_t loopCount = 0;
	VIDEO_FRAME_INFO_S stViFrame;
	int npu_cycle_time_ms = 1000 / rk_param_get_int("video.source:npu_fps", 10);

	long long before_time, cost_time;
	while (g_video_run_) {
		before_time = rkipc_get_curren_time_ms();
		ret = RK_MPI_VI_GetChnFrame(pipe_id_, VIDEO_PIPE_2, &stViFrame, 1000);
		if (ret == RK_SUCCESS) {
			void *data = RK_MPI_MB_Handle2VirAddr(stViFrame.stVFrame.pMbBlk);
			int32_t fd = RK_MPI_MB_Handle2Fd(stViFrame.stVFrame.pMbBlk);
			rkipc_rockiva_write_nv12_frame_by_fd(stViFrame.stVFrame.u32Width,
			                                     stViFrame.stVFrame.u32Height, loopCount, fd);
			ret = RK_MPI_VI_ReleaseChnFrame(pipe_id_, VIDEO_PIPE_2, &stViFrame);
			if (ret != RK_SUCCESS)
				LOG_ERROR("RK_MPI_VI_ReleaseChnFrame fail %x", ret);
			loopCount++;
		} else {
			LOG_ERROR("RK_MPI_VI_GetChnFrame timeout %x", ret);
		}
		cost_time = rkipc_get_curren_time_ms() - before_time;
		if ((cost_time > 0) && (cost_time < npu_cycle_time_ms))
			usleep((npu_cycle_time_ms - cost_time) * 1000);
	}
	return NULL;
}

static void *rkipc_ivs_get_results(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcGetIVS", 0, 0, 0);
	int ret, i;
	IVS_RESULT_INFO_S stResults;
	int resultscount = 0;
	int count = 0;
	int md = rk_param_get_int("ivs:md", 0);
	int od = rk_param_get_int("ivs:od", 0);
	int width = rk_param_get_int("video.2:width", 640);

	while (g_video_run_) {
		ret = RK_MPI_IVS_GetResults(0, &stResults, 1000);
		if (ret >= 0) {
			resultscount++;
			// LOG_DEBUG("get chn %d results %d\n", 0, resultscount);
			// LOG_DEBUG("get stResults.s32ResultNum %d\n", stResults.s32ResultNum);
			if (md == 1) {
				if (resultscount % 10 == 0 && stResults.s32ResultNum == 1) {
					int x = width / 8 / 8;
					int y = stResults.pstResults->stMdInfo.u32Size / 64;
					if (stResults.pstResults->stMdInfo.pData) {
						// for (int n = 0; n < x * 8; n++)
						// 	printf("-");
						// printf("\n");
						count = 0;
						for (int j = 0; j < y; j++) {
							for (int i = 0; i < x; i++) {
								for (int k = 0; k < 8; k++) {
									if (stResults.pstResults->stMdInfo.pData[j * 64 + i] & (1 << k))
										count++;
									// 	printf("1");
									// else
									// 	printf("0");
								}
							}
							// printf("\n");
						}
						// for (int n = 0; n < x * 8; n++)
						// 	printf("-");
						// printf("\n");
					}
					if (count > (x * y * 8 / 5)) {
						LOG_INFO("Detect movement\n");
					}
				}
			}
			if (od == 1) {
				if (stResults.s32ResultNum > 0) {
					if (stResults.pstResults->stOdInfo.u32Flag)
						LOG_INFO("OD flag:%d\n", stResults.pstResults->stOdInfo.u32Flag);
				}
			}
			RK_MPI_IVS_ReleaseResults(0, &stResults);
		} else {
			LOG_ERROR("get chn %d fail %d\n", 0, ret);
			usleep(50000llu);
		}
	}
	return NULL;
}


int rkipc_rtsp_init() {
	LOG_DEBUG("start\n");
	g_rtsplive = create_rtsp_demo(554);
	g_rtsp_session_0 = rtsp_new_session(g_rtsplive, RTSP_URL_0);
	g_rtsp_session_1 = rtsp_new_session(g_rtsplive, RTSP_URL_1);
	tmp_output_data_type = rk_param_get_string("video.0:output_data_type", "H.264");
	if (!strcmp(tmp_output_data_type, "H.264"))
		rtsp_set_video(g_rtsp_session_0, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
	else if (!strcmp(tmp_output_data_type, "H.265"))
		rtsp_set_video(g_rtsp_session_0, RTSP_CODEC_ID_VIDEO_H265, NULL, 0);
	else
		LOG_DEBUG("0 tmp_output_data_type is %s, not support\n", tmp_output_data_type);

	tmp_output_data_type = rk_param_get_string("video.1:output_data_type", "H.264");
	if (!strcmp(tmp_output_data_type, "H.264"))
		rtsp_set_video(g_rtsp_session_1, RTSP_CODEC_ID_VIDEO_H264, NULL, 0);
	else if (!strcmp(tmp_output_data_type, "H.265"))
		rtsp_set_video(g_rtsp_session_1, RTSP_CODEC_ID_VIDEO_H265, NULL, 0);
	else
		LOG_DEBUG("1 tmp_output_data_type is %s, not support\n", tmp_output_data_type);

	rtsp_sync_video_ts(g_rtsp_session_0, rtsp_get_reltime(), rtsp_get_ntptime());
	rtsp_sync_video_ts(g_rtsp_session_1, rtsp_get_reltime(), rtsp_get_ntptime());
	LOG_DEBUG("end\n");

	return 0;
}

int rkipc_rtsp_deinit() {
	LOG_DEBUG("%s\n", __func__);
	if (g_rtsp_session_0) {
		rtsp_del_session(g_rtsp_session_0);
		g_rtsp_session_0 = NULL;
	}
	if (g_rtsp_session_1) {
		rtsp_del_session(g_rtsp_session_1);
		g_rtsp_session_1 = NULL;
	}
	if (g_rtsplive)
		rtsp_del_demo(g_rtsplive);
	g_rtsplive = NULL;
	return 0;
}

int rkipc_vi_dev_init() {
	LOG_DEBUG("%s\n", __func__);
	int ret = 0;
	VI_DEV_ATTR_S stDevAttr;
	VI_DEV_BIND_PIPE_S stBindPipe;
	memset(&stDevAttr, 0, sizeof(stDevAttr));
	memset(&stBindPipe, 0, sizeof(stBindPipe));
	// 0. get dev config status
	ret = RK_MPI_VI_GetDevAttr(dev_id_, &stDevAttr);
	if (ret == RK_ERR_VI_NOT_CONFIG) {
		// 0-1.config dev
		ret = RK_MPI_VI_SetDevAttr(dev_id_, &stDevAttr);
		if (ret != RK_SUCCESS) {
			LOG_ERROR("RK_MPI_VI_SetDevAttr %x\n", ret);
			return -1;
		}
	} else {
		LOG_ERROR("RK_MPI_VI_SetDevAttr already\n");
	}
	// 1.get dev enable status
	ret = RK_MPI_VI_GetDevIsEnable(dev_id_);
	if (ret != RK_SUCCESS) {
		// 1-2.enable dev
		ret = RK_MPI_VI_EnableDev(dev_id_);
		if (ret != RK_SUCCESS) {
			LOG_ERROR("RK_MPI_VI_EnableDev %x\n", ret);
			return -1;
		}
		// 1-3.bind dev/pipe
		stBindPipe.u32Num = pipe_id_;
		stBindPipe.PipeId[0] = pipe_id_;
		ret = RK_MPI_VI_SetDevBindPipe(dev_id_, &stBindPipe);
		if (ret != RK_SUCCESS) {
			LOG_ERROR("RK_MPI_VI_SetDevBindPipe %x\n", ret);
			return -1;
		}
	} else {
		LOG_ERROR("RK_MPI_VI_EnableDev already\n");
	}

	VI_DEV_STATUS_S dev_status;
	RK_MPI_VI_QueryDevStatus(dev_id_, &dev_status);
	printf("[%s]sensor dev %d %d-%d\n", __func__, dev_status.bProbeOk, dev_status.stSize.u32Height, dev_status.stSize.u32Width);

	return 0;
}

int rkipc_vi_dev_deinit() {
	RK_MPI_VI_DisableDev(pipe_id_);

	return 0;
}


int rkipc_pipe_0_init() {
	int ret;
	int video_width = rk_param_get_int("video.0:width", -1);
	int video_height = rk_param_get_int("video.0:height", -1);
	int video_max_width = rk_param_get_int("video.0:max_width", -1);
	int video_max_height = rk_param_get_int("video.0:max_height", -1);
	int buffer_line = rk_param_get_int("video.source:buffer_line", video_max_height / 4);
	if (buffer_line < 128)
		buffer_line = video_max_height;
	int rotation = rk_param_get_int("video.source:rotation", 0);
	int buf_cnt = 2;
	int frame_min_i_qp = rk_param_get_int("video.0:frame_min_i_qp", 26);
	int frame_min_qp = rk_param_get_int("video.0:frame_min_qp", 28);
	int frame_max_i_qp = rk_param_get_int("video.0:frame_max_i_qp", 51);
	int frame_max_qp = rk_param_get_int("video.0:frame_max_qp", 51);
	int scalinglist = rk_param_get_int("video.0:scalinglist", 0);

	// VI
	VI_CHN_ATTR_S vi_chn_attr;
	memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
	vi_chn_attr.stIspOpt.u32BufCount = buf_cnt;
	vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
	vi_chn_attr.stIspOpt.stMaxSize.u32Width = rk_param_get_int("video.0:max_width", 2560);
	vi_chn_attr.stIspOpt.stMaxSize.u32Height = rk_param_get_int("video.0:max_height", 1440);
	vi_chn_attr.stSize.u32Width = video_width;
	vi_chn_attr.stSize.u32Height = video_height;
	vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
	vi_chn_attr.u32Depth = 0;
	vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE;
	ret = RK_MPI_VI_SetChnAttr(pipe_id_, VIDEO_PIPE_0, &vi_chn_attr);
	if (ret) {
		LOG_ERROR("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	VI_CHN_BUF_WRAP_S stViWrap;
	memset(&stViWrap, 0, sizeof(VI_CHN_BUF_WRAP_S));
	if (enable_wrap) {
		if (buffer_line < 128 || buffer_line > video_max_height) {
			LOG_ERROR("wrap mode buffer line must between [128, H], set as video_max_height\n");
			buffer_line = video_max_height;
		}
		stViWrap.bEnable = enable_wrap;
		stViWrap.u32BufLine = buffer_line;
		stViWrap.u32WrapBufferSize = stViWrap.u32BufLine * video_max_width * 3 / 2;
		LOG_INFO("set vi channel wrap line: %d, wrapBuffSize = %d\n", stViWrap.u32BufLine,
		         stViWrap.u32WrapBufferSize);
		RK_MPI_VI_SetChnWrapBufAttr(pipe_id_, VIDEO_PIPE_0, &stViWrap);
	}

	ret = RK_MPI_VI_EnableChn(pipe_id_, VIDEO_PIPE_0);
	if (ret) {
		LOG_ERROR("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	// VENC
	VENC_CHN_ATTR_S venc_chn_attr;
	memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
	tmp_output_data_type = rk_param_get_string("video.0:output_data_type", NULL);
	tmp_rc_mode = rk_param_get_string("video.0:rc_mode", NULL);
	tmp_h264_profile = rk_param_get_string("video.0:h264_profile", NULL);
	if ((tmp_output_data_type == NULL) || (tmp_rc_mode == NULL)) {
		LOG_ERROR("tmp_output_data_type or tmp_rc_mode is NULL\n");
		return -1;
	}
	LOG_DEBUG("tmp_output_data_type is %s, tmp_rc_mode is %s, tmp_h264_profile is %s\n",
	          tmp_output_data_type, tmp_rc_mode, tmp_h264_profile);
	if (!strcmp(tmp_output_data_type, "H.264")) {
		venc_chn_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;
		if (!strcmp(tmp_h264_profile, "high"))
			venc_chn_attr.stVencAttr.u32Profile = 100;
		else if (!strcmp(tmp_h264_profile, "main"))
			venc_chn_attr.stVencAttr.u32Profile = 77;
		else if (!strcmp(tmp_h264_profile, "baseline"))
			venc_chn_attr.stVencAttr.u32Profile = 66;
		else
			LOG_ERROR("tmp_h264_profile is %s\n", tmp_h264_profile);
		if (!strcmp(tmp_rc_mode, "CBR")) {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
			venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = rk_param_get_int("video.0:gop", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = rk_param_get_int("video.0:max_rate", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.0:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.0:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.0:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.0:src_frame_rate_num", -1);
		} else {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
			venc_chn_attr.stRcAttr.stH264Vbr.u32Gop = rk_param_get_int("video.0:gop", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32BitRate = rk_param_get_int("video.0:mid_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32MaxBitRate =
			    rk_param_get_int("video.0:max_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32MinBitRate =
			    rk_param_get_int("video.0:min_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.0:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.0:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.0:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.0:src_frame_rate_num", -1);
		}
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		venc_chn_attr.stVencAttr.enType = RK_VIDEO_ID_HEVC;
		if (!strcmp(tmp_rc_mode, "CBR")) {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
			venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = rk_param_get_int("video.0:gop", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = rk_param_get_int("video.0:max_rate", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.0:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.0:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.0:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.0:src_frame_rate_num", -1);
		} else {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
			venc_chn_attr.stRcAttr.stH265Vbr.u32Gop = rk_param_get_int("video.0:gop", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32BitRate = rk_param_get_int("video.0:mid_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32MaxBitRate =
			    rk_param_get_int("video.0:max_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32MinBitRate =
			    rk_param_get_int("video.0:min_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.0:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.0:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.0:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.0:src_frame_rate_num", -1);
		}
	} else {
		LOG_ERROR("tmp_output_data_type is %s, not support\n", tmp_output_data_type);
		return -1;
	}
	tmp_smart = rk_param_get_string("video.0:smart", NULL);
	tmp_gop_mode = rk_param_get_string("video.0:gop_mode", NULL);
	if (!strcmp(tmp_gop_mode, "normalP")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	} else if (!strcmp(tmp_gop_mode, "smartP")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
		venc_chn_attr.stGopAttr.s32VirIdrLen = rk_param_get_int("video.0:smartp_viridrlen", 25);
		venc_chn_attr.stGopAttr.u32MaxLtrCount = 1; // long-term reference frame ltr is fixed to 1
	} else if (!strcmp(tmp_gop_mode, "tsvc4")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_TSVC4;
	}
	// venc_chn_attr.stGopAttr.u32GopSize = rk_param_get_int("video.0:gop", -1);
	venc_chn_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
	venc_chn_attr.stVencAttr.u32MaxPicWidth = rk_param_get_int("video.0:max_width", 2560);
	venc_chn_attr.stVencAttr.u32MaxPicHeight = rk_param_get_int("video.0:max_height", 1440);
	venc_chn_attr.stVencAttr.u32PicWidth = video_width;
	venc_chn_attr.stVencAttr.u32PicHeight = video_height;
	venc_chn_attr.stVencAttr.u32VirWidth = video_width;
	venc_chn_attr.stVencAttr.u32VirHeight = video_height;
	venc_chn_attr.stVencAttr.u32StreamBufCnt = rk_param_get_int("video.0:buffer_count", 4);
	venc_chn_attr.stVencAttr.u32BufSize = rk_param_get_int("video.0:buffer_size", 1843200);
	// venc_chn_attr.stVencAttr.u32Depth = 1;
	ret = RK_MPI_VENC_CreateChn(VIDEO_PIPE_0, &venc_chn_attr);
	if (ret) {
		LOG_ERROR("ERROR: create VENC error! ret=%#x\n", ret);
		return -1;
	}

	if (!strcmp(tmp_smart, "open"))
		RK_MPI_VENC_EnableSvc(VIDEO_PIPE_0, 1);

	if (rk_param_get_int("video.0:enable_motion_deblur", 0)) {
		ret = RK_MPI_VENC_EnableMotionDeblur(VIDEO_PIPE_0, true);
		if (ret)
			LOG_ERROR("RK_MPI_VENC_EnableMotionDeblur error! ret=%#x\n", ret);
	}
	if (rk_param_get_int("video.0:enable_motion_static_switch", 0)) {
		ret = RK_MPI_VENC_EnableMotionStaticSwitch(VIDEO_PIPE_0, true);
		if (ret)
			LOG_ERROR("RK_MPI_VENC_EnableMotionStaticSwitch error! ret=%#x\n", ret);
	}

	// VENC_RC_PARAM_S h265_RcParam;
	// RK_MPI_VENC_GetRcParam(VIDEO_PIPE_0, &h265_RcParam);
	// h265_RcParam.s32FirstFrameStartQp = 26;
	// h265_RcParam.stParamH265.u32StepQp = 8;
	// h265_RcParam.stParamH265.u32MaxQp = 51;
	// h265_RcParam.stParamH265.u32MinQp = 10;
	// h265_RcParam.stParamH265.u32MaxIQp = 46;
	// h265_RcParam.stParamH265.u32MinIQp = 24;
	// h265_RcParam.stParamH265.s32DeltIpQp = -4;
	// RK_MPI_VENC_SetRcParam(VIDEO_PIPE_0, &h265_RcParam);

	tmp_rc_quality = rk_param_get_string("video.0:rc_quality", NULL);
	VENC_RC_PARAM_S venc_rc_param;
	RK_MPI_VENC_GetRcParam(VIDEO_PIPE_0, &venc_rc_param);
	if (!strcmp(tmp_output_data_type, "H.264")) {
		if (!strcmp(tmp_rc_quality, "highest")) {
			venc_rc_param.stParamH264.u32MinQp = 10;
		} else if (!strcmp(tmp_rc_quality, "higher")) {
			venc_rc_param.stParamH264.u32MinQp = 15;
		} else if (!strcmp(tmp_rc_quality, "high")) {
			venc_rc_param.stParamH264.u32MinQp = 20;
		} else if (!strcmp(tmp_rc_quality, "medium")) {
			venc_rc_param.stParamH264.u32MinQp = 25;
		} else if (!strcmp(tmp_rc_quality, "low")) {
			venc_rc_param.stParamH264.u32MinQp = 30;
		} else if (!strcmp(tmp_rc_quality, "lower")) {
			venc_rc_param.stParamH264.u32MinQp = 35;
		} else {
			venc_rc_param.stParamH264.u32MinQp = 40;
		}
		venc_rc_param.stParamH264.u32FrmMinIQp = frame_min_i_qp;
		venc_rc_param.stParamH264.u32FrmMinQp = frame_min_qp;
		venc_rc_param.stParamH264.u32FrmMaxIQp = frame_max_i_qp;
		venc_rc_param.stParamH264.u32FrmMaxQp = frame_max_qp;
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		if (!strcmp(tmp_rc_quality, "highest")) {
			venc_rc_param.stParamH265.u32MinQp = 10;
		} else if (!strcmp(tmp_rc_quality, "higher")) {
			venc_rc_param.stParamH265.u32MinQp = 15;
		} else if (!strcmp(tmp_rc_quality, "high")) {
			venc_rc_param.stParamH265.u32MinQp = 20;
		} else if (!strcmp(tmp_rc_quality, "medium")) {
			venc_rc_param.stParamH265.u32MinQp = 25;
		} else if (!strcmp(tmp_rc_quality, "low")) {
			venc_rc_param.stParamH265.u32MinQp = 30;
		} else if (!strcmp(tmp_rc_quality, "lower")) {
			venc_rc_param.stParamH265.u32MinQp = 35;
		} else {
			venc_rc_param.stParamH265.u32MinQp = 40;
		}
		venc_rc_param.stParamH265.u32FrmMinIQp = frame_min_i_qp;
		venc_rc_param.stParamH265.u32FrmMinQp = frame_min_qp;
		venc_rc_param.stParamH265.u32FrmMaxIQp = frame_max_i_qp;
		venc_rc_param.stParamH265.u32FrmMaxQp = frame_max_qp;
	} else {
		LOG_ERROR("tmp_output_data_type is %s, not support\n", tmp_output_data_type);
		return -1;
	}
	RK_MPI_VENC_SetRcParam(VIDEO_PIPE_0, &venc_rc_param);

	VENC_CHN_BUF_WRAP_S stVencChnBufWrap;
	memset(&stVencChnBufWrap, 0, sizeof(stVencChnBufWrap));
	if (enable_wrap) {
		stVencChnBufWrap.bEnable = enable_wrap;
		stVencChnBufWrap.u32BufLine =
		    rk_param_get_int("video.source:buffer_line", video_max_height);
		if (stVencChnBufWrap.u32BufLine < 128)
			stVencChnBufWrap.u32BufLine = video_max_height;
		RK_MPI_VENC_SetChnBufWrapAttr(VIDEO_PIPE_0, &stVencChnBufWrap);
	}

	if (!strcmp(tmp_output_data_type, "H.264")) {
		VENC_H264_TRANS_S pstH264Trans;
		RK_MPI_VENC_GetH264Trans(VIDEO_PIPE_0, &pstH264Trans);
		pstH264Trans.bScalingListValid = scalinglist;
		RK_MPI_VENC_SetH264Trans(VIDEO_PIPE_0, &pstH264Trans);
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		VENC_H265_TRANS_S pstH265Trans;
		RK_MPI_VENC_GetH265Trans(VIDEO_PIPE_0, &pstH265Trans);
		pstH265Trans.bScalingListEnabled = scalinglist;
		RK_MPI_VENC_SetH265Trans(VIDEO_PIPE_0, &pstH265Trans);
	}
	VENC_CHN_REF_BUF_SHARE_S stVencChnRefBufShare;
	memset(&stVencChnRefBufShare, 0, sizeof(VENC_CHN_REF_BUF_SHARE_S));
	stVencChnRefBufShare.bEnable = rk_param_get_int("video.0:enable_refer_buffer_share", 0);
	RK_MPI_VENC_SetChnRefBufShareAttr(VIDEO_PIPE_0, &stVencChnRefBufShare);
	if (rotation == 0) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_0, ROTATION_0);
	} else if (rotation == 90) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_0, ROTATION_90);
	} else if (rotation == 180) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_0, ROTATION_180);
	} else if (rotation == 270) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_0, ROTATION_270);
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = -1;
	RK_MPI_VENC_StartRecvFrame(VIDEO_PIPE_0, &stRecvParam);
	pthread_create(&venc_thread_0, NULL, rkipc_get_venc_0, NULL);
	// bind
	vi_chn.enModId = RK_ID_VI;
	vi_chn.s32DevId = 0;
	vi_chn.s32ChnId = VIDEO_PIPE_0;
	venc_chn.enModId = RK_ID_VENC;
	venc_chn.s32DevId = 0;
	venc_chn.s32ChnId = VIDEO_PIPE_0;
	ret = RK_MPI_SYS_Bind(&vi_chn, &venc_chn);
	if (ret)
		LOG_ERROR("Bind VI and VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("Bind VI and VENC success\n");

	return 0;
}

int rkipc_pipe_0_deinit() {
	int ret;
	// unbind
	vi_chn.enModId = RK_ID_VI;
	vi_chn.s32DevId = 0;
	vi_chn.s32ChnId = VIDEO_PIPE_0;
	venc_chn.enModId = RK_ID_VENC;
	venc_chn.s32DevId = 0;
	venc_chn.s32ChnId = VIDEO_PIPE_0;
	ret = RK_MPI_SYS_UnBind(&vi_chn, &venc_chn);
	if (ret)
		LOG_ERROR("Unbind VI and VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("Unbind VI and VENC success\n");
	// VENC
	ret = RK_MPI_VENC_StopRecvFrame(VIDEO_PIPE_0);
	ret |= RK_MPI_VENC_DestroyChn(VIDEO_PIPE_0);
	if (ret)
		LOG_ERROR("ERROR: Destroy VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("RK_MPI_VENC_DestroyChn success\n");
	// VI
	ret = RK_MPI_VI_DisableChn(pipe_id_, VIDEO_PIPE_0);
	if (ret)
		LOG_ERROR("ERROR: Destroy VI error! ret=%#x\n", ret);

	return 0;
}


int rkipc_pipe_1_init() {
	int ret;
	int video_width = rk_param_get_int("video.1:width", 1920);
	int video_height = rk_param_get_int("video.1:height", 1080);
	int buf_cnt = rk_param_get_int("video.1:input_buffer_count", 2);
	int rotation = rk_param_get_int("video.source:rotation", 0);
	int frame_min_i_qp = rk_param_get_int("video.1:frame_min_i_qp", 26);
	int frame_min_qp = rk_param_get_int("video.1:frame_min_qp", 28);
	int frame_max_i_qp = rk_param_get_int("video.1:frame_max_i_qp", 51);
	int frame_max_qp = rk_param_get_int("video.1:frame_max_qp", 51);
	int scalinglist = rk_param_get_int("video.1:scalinglist", 0);

	// VI
	VI_CHN_ATTR_S vi_chn_attr;
	memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
	vi_chn_attr.stIspOpt.u32BufCount = buf_cnt;
	vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
	vi_chn_attr.stIspOpt.stMaxSize.u32Width = rk_param_get_int("video.1:max_width", 704);
	vi_chn_attr.stIspOpt.stMaxSize.u32Height = rk_param_get_int("video.1:max_height", 576);
	vi_chn_attr.stSize.u32Width = video_width;
	vi_chn_attr.stSize.u32Height = video_height;
	vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
	vi_chn_attr.u32Depth = 0;
	ret = RK_MPI_VI_SetChnAttr(pipe_id_, VIDEO_PIPE_1, &vi_chn_attr);
	ret |= RK_MPI_VI_EnableChn(pipe_id_, VIDEO_PIPE_1);
	if (ret) {
		LOG_ERROR("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	// VENC
	VENC_CHN_ATTR_S venc_chn_attr;
	memset(&venc_chn_attr, 0, sizeof(venc_chn_attr));
	tmp_output_data_type = rk_param_get_string("video.1:output_data_type", NULL);
	tmp_rc_mode = rk_param_get_string("video.1:rc_mode", NULL);
	tmp_h264_profile = rk_param_get_string("video.1:h264_profile", NULL);
	if ((tmp_output_data_type == NULL) || (tmp_rc_mode == NULL)) {
		LOG_ERROR("tmp_output_data_type or tmp_rc_mode is NULL\n");
		return -1;
	}
	LOG_DEBUG("tmp_output_data_type is %s, tmp_rc_mode is %s, tmp_h264_profile is %s\n",
	          tmp_output_data_type, tmp_rc_mode, tmp_h264_profile);
	if (!strcmp(tmp_output_data_type, "H.264")) {
		venc_chn_attr.stVencAttr.enType = RK_VIDEO_ID_AVC;
		if (!strcmp(tmp_h264_profile, "high"))
			venc_chn_attr.stVencAttr.u32Profile = 100;
		else if (!strcmp(tmp_h264_profile, "main"))
			venc_chn_attr.stVencAttr.u32Profile = 77;
		else if (!strcmp(tmp_h264_profile, "baseline"))
			venc_chn_attr.stVencAttr.u32Profile = 66;
		else
			LOG_ERROR("tmp_h264_profile is %s\n", tmp_h264_profile);
		if (!strcmp(tmp_rc_mode, "CBR")) {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264CBR;
			venc_chn_attr.stRcAttr.stH264Cbr.u32Gop = rk_param_get_int("video.1:gop", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32BitRate = rk_param_get_int("video.1:max_rate", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.1:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.1:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.1:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Cbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.1:src_frame_rate_num", -1);
		} else {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H264VBR;
			venc_chn_attr.stRcAttr.stH264Vbr.u32Gop = rk_param_get_int("video.1:gop", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32BitRate = rk_param_get_int("video.1:mid_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32MaxBitRate =
			    rk_param_get_int("video.1:max_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32MinBitRate =
			    rk_param_get_int("video.1:min_rate", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.1:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.1:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.1:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH264Vbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.1:src_frame_rate_num", -1);
		}
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		venc_chn_attr.stVencAttr.enType = RK_VIDEO_ID_HEVC;
		if (!strcmp(tmp_rc_mode, "CBR")) {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265CBR;
			venc_chn_attr.stRcAttr.stH265Cbr.u32Gop = rk_param_get_int("video.1:gop", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32BitRate = rk_param_get_int("video.1:max_rate", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.1:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.1:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.1:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Cbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.1:src_frame_rate_num", -1);
		} else {
			venc_chn_attr.stRcAttr.enRcMode = VENC_RC_MODE_H265VBR;
			venc_chn_attr.stRcAttr.stH265Vbr.u32Gop = rk_param_get_int("video.1:gop", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32BitRate = rk_param_get_int("video.1:mid_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32MaxBitRate =
			    rk_param_get_int("video.1:max_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32MinBitRate =
			    rk_param_get_int("video.1:min_rate", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateDen =
			    rk_param_get_int("video.1:dst_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.fr32DstFrameRateNum =
			    rk_param_get_int("video.1:dst_frame_rate_num", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateDen =
			    rk_param_get_int("video.1:src_frame_rate_den", -1);
			venc_chn_attr.stRcAttr.stH265Vbr.u32SrcFrameRateNum =
			    rk_param_get_int("video.1:src_frame_rate_num", -1);
		}
	} else {
		LOG_ERROR("tmp_output_data_type is %s, not support\n", tmp_output_data_type);
		return -1;
	}
	tmp_smart = rk_param_get_string("video.1:smart", NULL);
	tmp_gop_mode = rk_param_get_string("video.1:gop_mode", NULL);
	if (!strcmp(tmp_gop_mode, "normalP")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_NORMALP;
	} else if (!strcmp(tmp_gop_mode, "smartP")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_SMARTP;
		venc_chn_attr.stGopAttr.s32VirIdrLen = rk_param_get_int("video.1:smartp_viridrlen", 25);
		venc_chn_attr.stGopAttr.u32MaxLtrCount = 1; // long-term reference frame ltr is fixed to 1
	} else if (!strcmp(tmp_gop_mode, "tsvc4")) {
		venc_chn_attr.stGopAttr.enGopMode = VENC_GOPMODE_TSVC4;
	}
	// venc_chn_attr.stGopAttr.u32GopSize = rk_param_get_int("video.1:gop", -1);

	venc_chn_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
	venc_chn_attr.stVencAttr.u32MaxPicWidth = rk_param_get_int("video.1:max_width", 704);
	venc_chn_attr.stVencAttr.u32MaxPicHeight = rk_param_get_int("video.1:max_height", 576);
	venc_chn_attr.stVencAttr.u32PicWidth = video_width;
	venc_chn_attr.stVencAttr.u32PicHeight = video_height;
	venc_chn_attr.stVencAttr.u32VirWidth = video_width;
	venc_chn_attr.stVencAttr.u32VirHeight = video_height;
	venc_chn_attr.stVencAttr.u32StreamBufCnt = rk_param_get_int("video.1:buffer_count", 4);
	venc_chn_attr.stVencAttr.u32BufSize = rk_param_get_int("video.1:buffer_size", 202752);
	// venc_chn_attr.stVencAttr.u32Depth = 1;
	ret = RK_MPI_VENC_CreateChn(VIDEO_PIPE_1, &venc_chn_attr);
	if (ret) {
		LOG_ERROR("ERROR: create VENC error! ret=%#x\n", ret);
		return -1;
	}

	if (!strcmp(tmp_smart, "open"))
		RK_MPI_VENC_EnableSvc(VIDEO_PIPE_1, 1);

	if (rk_param_get_int("video.1:enable_motion_deblur", 0)) {
		ret = RK_MPI_VENC_EnableMotionDeblur(VIDEO_PIPE_1, true);
		if (ret)
			LOG_ERROR("RK_MPI_VENC_EnableMotionDeblur error! ret=%#x\n", ret);
	}
	if (rk_param_get_int("video.1:enable_motion_static_switch", 0)) {
		ret = RK_MPI_VENC_EnableMotionStaticSwitch(VIDEO_PIPE_1, true);
		if (ret)
			LOG_ERROR("RK_MPI_VENC_EnableMotionStaticSwitch error! ret=%#x\n", ret);
	}

	tmp_rc_quality = rk_param_get_string("video.1:rc_quality", NULL);
	VENC_RC_PARAM_S venc_rc_param;
	RK_MPI_VENC_GetRcParam(VIDEO_PIPE_1, &venc_rc_param);
	if (!strcmp(tmp_output_data_type, "H.264")) {
		if (!strcmp(tmp_rc_quality, "highest")) {
			venc_rc_param.stParamH264.u32MinQp = 10;
		} else if (!strcmp(tmp_rc_quality, "higher")) {
			venc_rc_param.stParamH264.u32MinQp = 15;
		} else if (!strcmp(tmp_rc_quality, "high")) {
			venc_rc_param.stParamH264.u32MinQp = 20;
		} else if (!strcmp(tmp_rc_quality, "medium")) {
			venc_rc_param.stParamH264.u32MinQp = 25;
		} else if (!strcmp(tmp_rc_quality, "low")) {
			venc_rc_param.stParamH264.u32MinQp = 30;
		} else if (!strcmp(tmp_rc_quality, "lower")) {
			venc_rc_param.stParamH264.u32MinQp = 35;
		} else {
			venc_rc_param.stParamH264.u32MinQp = 40;
		}
		venc_rc_param.stParamH264.u32FrmMinIQp = frame_min_i_qp;
		venc_rc_param.stParamH264.u32FrmMinQp = frame_min_qp;
		venc_rc_param.stParamH264.u32FrmMaxIQp = frame_max_i_qp;
		venc_rc_param.stParamH264.u32FrmMaxQp = frame_max_qp;
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		if (!strcmp(tmp_rc_quality, "highest")) {
			venc_rc_param.stParamH265.u32MinQp = 10;
		} else if (!strcmp(tmp_rc_quality, "higher")) {
			venc_rc_param.stParamH265.u32MinQp = 15;
		} else if (!strcmp(tmp_rc_quality, "high")) {
			venc_rc_param.stParamH265.u32MinQp = 20;
		} else if (!strcmp(tmp_rc_quality, "medium")) {
			venc_rc_param.stParamH265.u32MinQp = 25;
		} else if (!strcmp(tmp_rc_quality, "low")) {
			venc_rc_param.stParamH265.u32MinQp = 30;
		} else if (!strcmp(tmp_rc_quality, "lower")) {
			venc_rc_param.stParamH265.u32MinQp = 35;
		} else {
			venc_rc_param.stParamH265.u32MinQp = 40;
		}
		venc_rc_param.stParamH265.u32FrmMinIQp = frame_min_i_qp;
		venc_rc_param.stParamH265.u32FrmMinQp = frame_min_qp;
		venc_rc_param.stParamH265.u32FrmMaxIQp = frame_max_i_qp;
		venc_rc_param.stParamH265.u32FrmMaxQp = frame_max_qp;
	} else {
		LOG_ERROR("tmp_output_data_type is %s, not support\n", tmp_output_data_type);
		return -1;
	}
	RK_MPI_VENC_SetRcParam(VIDEO_PIPE_1, &venc_rc_param);

	if (!strcmp(tmp_output_data_type, "H.264")) {
		VENC_H264_TRANS_S pstH264Trans;
		RK_MPI_VENC_GetH264Trans(VIDEO_PIPE_1, &pstH264Trans);
		pstH264Trans.bScalingListValid = scalinglist;
		RK_MPI_VENC_SetH264Trans(VIDEO_PIPE_1, &pstH264Trans);
	} else if (!strcmp(tmp_output_data_type, "H.265")) {
		VENC_H265_TRANS_S pstH265Trans;
		RK_MPI_VENC_GetH265Trans(VIDEO_PIPE_1, &pstH265Trans);
		pstH265Trans.bScalingListEnabled = scalinglist;
		RK_MPI_VENC_SetH265Trans(VIDEO_PIPE_1, &pstH265Trans);
	}
	VENC_CHN_REF_BUF_SHARE_S stVencChnRefBufShare;
	memset(&stVencChnRefBufShare, 0, sizeof(VENC_CHN_REF_BUF_SHARE_S));
	stVencChnRefBufShare.bEnable = rk_param_get_int("video.1:enable_refer_buffer_share", 0);
	RK_MPI_VENC_SetChnRefBufShareAttr(VIDEO_PIPE_1, &stVencChnRefBufShare);
	if (rotation == 0) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_1, ROTATION_0);
	} else if (rotation == 90) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_1, ROTATION_90);
	} else if (rotation == 180) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_1, ROTATION_180);
	} else if (rotation == 270) {
		RK_MPI_VENC_SetChnRotation(VIDEO_PIPE_1, ROTATION_270);
	}

	VENC_RECV_PIC_PARAM_S stRecvParam;
	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = -1;
	RK_MPI_VENC_StartRecvFrame(VIDEO_PIPE_1, &stRecvParam);
	pthread_create(&venc_thread_1, NULL, rkipc_get_venc_1, NULL);

	// bind
	vi_chn.enModId = RK_ID_VI;
	vi_chn.s32DevId = 0;
	vi_chn.s32ChnId = VIDEO_PIPE_1;
	venc_chn.enModId = RK_ID_VENC;
	venc_chn.s32DevId = 0;
	venc_chn.s32ChnId = VIDEO_PIPE_1;
	ret = RK_MPI_SYS_Bind(&vi_chn, &venc_chn);
	if (ret)
		LOG_ERROR("Bind VI and VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("Bind VI and VENC success\n");

	return 0;
}

int rkipc_pipe_1_deinit() {
	int ret;
	// unbind
	vi_chn.enModId = RK_ID_VI;
	vi_chn.s32DevId = 0;
	vi_chn.s32ChnId = VIDEO_PIPE_1;
	venc_chn.enModId = RK_ID_VENC;
	venc_chn.s32DevId = 0;
	venc_chn.s32ChnId = VIDEO_PIPE_1;
	ret = RK_MPI_SYS_UnBind(&vi_chn, &venc_chn);
	if (ret)
		LOG_ERROR("Unbind VI and VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("Unbind VI and VENC success\n");
	// VENC
	ret = RK_MPI_VENC_StopRecvFrame(VIDEO_PIPE_1);
	ret |= RK_MPI_VENC_DestroyChn(VIDEO_PIPE_1);
	if (ret)
		LOG_ERROR("ERROR: Destroy VENC error! ret=%#x\n", ret);
	else
		LOG_DEBUG("RK_MPI_VENC_DestroyChn success\n");
	// VI
	ret = RK_MPI_VI_DisableChn(pipe_id_, VIDEO_PIPE_1);
	if (ret)
		LOG_ERROR("ERROR: Destroy VI error! ret=%#x\n", ret);

	return 0;
}


static RK_U8 rgn_color_lut_0_left_value[4] = {0x03, 0xf, 0x3f, 0xff};
static RK_U8 rgn_color_lut_0_right_value[4] = {0xc0, 0xf0, 0xfc, 0xff};
static RK_U8 rgn_color_lut_1_left_value[4] = {0x02, 0xa, 0x2a, 0xaa};
static RK_U8 rgn_color_lut_1_right_value[4] = {0x80, 0xa0, 0xa8, 0xaa};
RK_S32 draw_rect_2bpp(RK_U8 *buffer, RK_U32 width, RK_U32 height, int rgn_x, int rgn_y, int rgn_w,
                      int rgn_h, int line_pixel, COLOR_INDEX_E color_index) {
	int i;
	RK_U8 *ptr = buffer;
	RK_U8 value = 0;
	if (color_index == RGN_COLOR_LUT_INDEX_0)
		value = 0xff;
	if (color_index == RGN_COLOR_LUT_INDEX_1)
		value = 0xaa;

	if (line_pixel > 4) {
		printf("line_pixel > 4, not support\n", line_pixel);
		return -1;
	}

	// printf("YUV %dx%d, rgn (%d,%d,%d,%d), line pixel %d\n", width, height, rgn_x, rgn_y, rgn_w,
	// rgn_h, line_pixel); draw top line
	ptr += (width * rgn_y + rgn_x) >> 2;
	for (i = 0; i < line_pixel; i++) {
		memset(ptr, value, (rgn_w + 3) >> 2);
		ptr += width >> 2;
	}
	// draw letft/right line
	for (i = 0; i < (rgn_h - line_pixel * 2); i++) {
		if (color_index == RGN_COLOR_LUT_INDEX_1) {
			*ptr = rgn_color_lut_1_left_value[line_pixel - 1];
			*(ptr + ((rgn_w + 3) >> 2)) = rgn_color_lut_1_right_value[line_pixel - 1];
		} else {
			*ptr = rgn_color_lut_0_left_value[line_pixel - 1];
			*(ptr + ((rgn_w + 3) >> 2)) = rgn_color_lut_0_right_value[line_pixel - 1];
		}
		ptr += width >> 2;
	}
	// draw bottom line
	for (i = 0; i < line_pixel; i++) {
		memset(ptr, value, (rgn_w + 3) >> 2);
		ptr += width >> 2;
	}
	return 0;
}

static void *rkipc_get_nn_update_osd(void *arg) {
	LOG_DEBUG("#Start %s thread, arg:%p\n", __func__, arg);
	prctl(PR_SET_NAME, "RkipcNpuOsd", 0, 0, 0);

	int ret = 0;
	int line_pixel = 2;
	int change_to_nothing_flag = 0;
	int video_width = 0;
	int video_height = 0;
	int rotation = 0;
	long long last_ba_result_time;
	RockIvaBaResult ba_result;
	RockIvaBaObjectInfo *object;
	RGN_HANDLE RgnHandle = DRAW_NN_OSD_ID;
	RGN_CANVAS_INFO_S stCanvasInfo;

	memset(&stCanvasInfo, 0, sizeof(RGN_CANVAS_INFO_S));
	memset(&ba_result, 0, sizeof(ba_result));
	while (g_nn_osd_run_) {
		usleep(40 * 1000);
		rotation = rk_param_get_int("video.source:rotation", 0);
		if (rotation == 90 || rotation == 270) {
			video_width = rk_param_get_int("video.0:height", -1);
			video_height = rk_param_get_int("video.0:width", -1);
		} else {
			video_width = rk_param_get_int("video.0:width", -1);
			video_height = rk_param_get_int("video.0:height", -1);
		}
		ret = rkipc_rknn_object_get(&ba_result);
		// LOG_DEBUG("ret is %d, ba_result.objNum is %d\n", ret, ba_result.objNum);

		if ((ret == -1) && (rkipc_get_curren_time_ms() - last_ba_result_time > 300))
			ba_result.objNum = 0;
		if (ret == 0)
			last_ba_result_time = rkipc_get_curren_time_ms();

		ret = RK_MPI_RGN_GetCanvasInfo(RgnHandle, &stCanvasInfo);
		if (ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_RGN_GetCanvasInfo failed with %#x!", ret);
			continue;
		}
		if ((stCanvasInfo.stSize.u32Width != UPALIGNTO16(video_width)) ||
		    (stCanvasInfo.stSize.u32Height != UPALIGNTO16(video_height))) {
			LOG_WARN("canvas is %d*%d, not equal %d*%d, maybe in the process of switching,"
			         "skip this time\n",
			         stCanvasInfo.stSize.u32Width, stCanvasInfo.stSize.u32Height,
			         UPALIGNTO16(video_width), UPALIGNTO16(video_height));
			continue;
		}
		memset((void *)stCanvasInfo.u64VirAddr, 0,
		       stCanvasInfo.u32VirWidth * stCanvasInfo.u32VirHeight >> 2);
		// draw
		for (int i = 0; i < ba_result.objNum; i++) {
			int x, y, w, h;
			object = &ba_result.triggerObjects[i];
			// LOG_INFO("topLeft:[%d,%d], bottomRight:[%d,%d],"
			// 			"objId is %d, frameId is %d, score is %d, type is %d\n",
			// 			object->objInfo.rect.topLeft.x, object->objInfo.rect.topLeft.y,
			// 			object->objInfo.rect.bottomRight.x,
			// 			object->objInfo.rect.bottomRight.y, object->objInfo.objId,
			// 			object->objInfo.frameId, object->objInfo.score, object->objInfo.type);
			x = video_width * object->objInfo.rect.topLeft.x / 10000;
			y = video_height * object->objInfo.rect.topLeft.y / 10000;
			w = video_width *
			    (object->objInfo.rect.bottomRight.x - object->objInfo.rect.topLeft.x) / 10000;
			h = video_height *
			    (object->objInfo.rect.bottomRight.y - object->objInfo.rect.topLeft.y) / 10000;
			x = x / 16 * 16;
			y = y / 16 * 16;
			w = (w + 3) / 16 * 16;
			h = (h + 3) / 16 * 16;

			while (x + w + line_pixel >= video_width) {
				w -= 8;
			}
			while (y + h + line_pixel >= video_height) {
				h -= 8;
			}
			if (x < 0 || y < 0 || w < 0 || h < 0) {
				continue;
			}
			// LOG_DEBUG("i is %d, x,y,w,h is %d,%d,%d,%d\n", i, x, y, w, h);
			if (object->objInfo.type == ROCKIVA_OBJECT_TYPE_PERSON) {
				draw_rect_2bpp((RK_U8 *)stCanvasInfo.u64VirAddr, stCanvasInfo.u32VirWidth,
				               stCanvasInfo.u32VirHeight, x, y, w, h, line_pixel,
				               RGN_COLOR_LUT_INDEX_0);
			} else if (object->objInfo.type == ROCKIVA_OBJECT_TYPE_FACE) {
				draw_rect_2bpp((RK_U8 *)stCanvasInfo.u64VirAddr, stCanvasInfo.u32VirWidth,
				               stCanvasInfo.u32VirHeight, x, y, w, h, line_pixel,
				               RGN_COLOR_LUT_INDEX_0);
			}
			// LOG_INFO("draw rect time-consuming is %lld\n",(rkipc_get_curren_time_ms() -
			// 	last_ba_result_time));
			// LOG_INFO("triggerRules is %d, ruleID is %d, triggerType is %d\n",
			// 			object->triggerRules,
			// 			object->firstTrigger.ruleID,
			// 			object->firstTrigger.triggerType);
		}
		ret = RK_MPI_RGN_UpdateCanvas(RgnHandle);
		if (ret != RK_SUCCESS) {
			RK_LOGE("RK_MPI_RGN_UpdateCanvas failed with %#x!", ret);
			continue;
		}
	}

	return 0;
}

int rkipc_osd_draw_nn_init() {
	LOG_DEBUG("start\n");
	int ret = 0;
	RGN_HANDLE RgnHandle = DRAW_NN_OSD_ID;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stRgnChnAttr;
	BITMAP_S stBitmap;
	int rotation = rk_param_get_int("video.source:rotation", 0);

	// create overlay regions
	memset(&stRgnAttr, 0, sizeof(stRgnAttr));
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = RK_FMT_2BPP;
	stRgnAttr.unAttr.stOverlay.u32CanvasNum = 1;
	if (rotation == 90 || rotation == 270) {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:max_height", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:max_width", -1);
	} else {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:max_width", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:max_height", -1);
	}
	ret = RK_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Create (%d) failed with %#x\n", RgnHandle, ret);
		RK_MPI_RGN_Destroy(RgnHandle);
		return RK_FAILURE;
	}
	LOG_DEBUG("The handle: %d, create success\n", RgnHandle);
	// after malloc max size, it needs to be set to the actual size
	if (rotation == 90 || rotation == 270) {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:height", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:width", -1);
	} else {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:width", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:height", -1);
	}
	ret = RK_MPI_RGN_SetAttr(RgnHandle, &stRgnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_SetAttr (%d) failed with %#x!", RgnHandle, ret);
		return RK_FAILURE;
	}

	// display overlay regions to venc groups
	memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
	stRgnChnAttr.bShow = RK_TRUE;
	stRgnChnAttr.enType = OVERLAY_RGN;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = DRAW_NN_OSD_ID;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[RGN_COLOR_LUT_INDEX_0] = BLUE_COLOR;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[RGN_COLOR_LUT_INDEX_1] = RED_COLOR;
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = DRAW_NN_VENC_CHN_ID;
	ret = RK_MPI_RGN_AttachToChn(RgnHandle, &stMppChn, &stRgnChnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) to venc0 failed with %#x\n", RgnHandle, ret);
		return RK_FAILURE;
	}
	LOG_DEBUG("RK_MPI_RGN_AttachToChn to venc0 success\n");
	pthread_create(&get_nn_update_osd_thread_id, NULL, rkipc_get_nn_update_osd, NULL);
	LOG_DEBUG("end\n");

	return ret;
}

int rkipc_osd_draw_nn_deinit() {
	LOG_DEBUG("%s\n", __func__);
	int ret = 0;
	pthread_join(get_nn_update_osd_thread_id, NULL);
	// Detach osd from chn
	MPP_CHN_S stMppChn;
	RGN_HANDLE RgnHandle = DRAW_NN_OSD_ID;
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = DRAW_NN_VENC_CHN_ID;
	ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
	if (RK_SUCCESS != ret)
		LOG_ERROR("RK_MPI_RGN_DetachFrmChn (%d) to venc0 failed with %#x\n", RgnHandle, ret);

	// destory region
	ret = RK_MPI_RGN_Destroy(RgnHandle);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Destroy [%d] failed with %#x\n", RgnHandle, ret);
	}
	LOG_DEBUG("Destory handle:%d success\n", RgnHandle);

	return ret;
}

int rkipc_osd_draw_nn_change() {
	LOG_INFO("%s\n", __func__);
	int ret = 0;
	int rotation = rk_param_get_int("video.source:rotation", 0);
	MPP_CHN_S stMppChn;
	RGN_ATTR_S stRgnAttr;
	RGN_CHN_ATTR_S stRgnChnAttr;
	RGN_HANDLE RgnHandle = DRAW_NN_OSD_ID;
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = DRAW_NN_VENC_CHN_ID;
	ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
	if (RK_SUCCESS != ret)
		LOG_ERROR("RK_MPI_RGN_DetachFrmChn (%d) to venc0 failed with %#x\n", RgnHandle, ret);
	ret = RK_MPI_RGN_GetAttr(RgnHandle, &stRgnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_GetAttr (%d) failed with %#x!", RgnHandle, ret);
		return RK_FAILURE;
	}
	if (rotation == 90 || rotation == 270) {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:height", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:width", -1);
	} else {
		stRgnAttr.unAttr.stOverlay.stSize.u32Width = rk_param_get_int("video.0:width", -1);
		stRgnAttr.unAttr.stOverlay.stSize.u32Height = rk_param_get_int("video.0:height", -1);
	}
	ret = RK_MPI_RGN_SetAttr(RgnHandle, &stRgnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_SetAttr (%d) failed with %#x!", RgnHandle, ret);
		return RK_FAILURE;
	}

	memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
	stRgnChnAttr.bShow = RK_TRUE;
	stRgnChnAttr.enType = OVERLAY_RGN;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = 0;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = DRAW_NN_OSD_ID;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[RGN_COLOR_LUT_INDEX_0] = BLUE_COLOR;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32ColorLUT[RGN_COLOR_LUT_INDEX_1] = RED_COLOR;
	ret = RK_MPI_RGN_AttachToChn(RgnHandle, &stMppChn, &stRgnChnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) failed with %#x!", RgnHandle, ret);
		return RK_FAILURE;
	}

	return ret;
}

int rkipc_ivs_init() {
	int ret;
	int video_width = rk_param_get_int("video.2:width", -1);
	int video_height = rk_param_get_int("video.2:height", -1);
	int buf_cnt = 2;
	int smear = rk_param_get_int("ivs:smear", 1);
	int weightp = rk_param_get_int("ivs:weightp", 1);
	int md = rk_param_get_int("ivs:md", 0);
	int od = rk_param_get_int("ivs:od", 0);
	if (!smear && !weightp && !md && !od) {
		LOG_INFO("no pp function enabled! end\n");
		return -1;
	}

	// IVS
	IVS_CHN_ATTR_S attr;
	memset(&attr, 0, sizeof(attr));
	attr.enMode = IVS_MODE_MD_OD;
	attr.u32PicWidth = video_width;
	attr.u32PicHeight = video_height;
	attr.enPixelFormat = RK_FMT_YUV420SP;
	attr.s32Gop = 30;
	attr.bSmearEnable = smear;
	attr.bWeightpEnable = weightp;
	attr.bMDEnable = md;
	attr.s32MDInterval = 1;
	attr.bMDNightMode = RK_TRUE;
	attr.u32MDSensibility = rk_param_get_int("ivs:md_sensibility", 3);
	attr.bODEnable = od;
	attr.s32ODInterval = 1;
	attr.s32ODPercent = 7;
	RK_MPI_IVS_CreateChn(0, &attr);

	if (md == 1 || od == 1)
		pthread_create(&get_ivs_result_thread, NULL, rkipc_ivs_get_results, NULL);

	return 0;
}

int rkipc_ivs_deinit() {
	int ret;
	pthread_join(get_ivs_result_thread, NULL);
	// IVS
	ret = RK_MPI_IVS_DestroyChn(0);
	if (ret)
		LOG_ERROR("ERROR: Destroy IVS error! ret=%#x\n", ret);
	else
		LOG_DEBUG("RK_MPI_IVS_DestroyChn success\n");

	return 0;
}

int rkipc_pipe_2_init() {
	int ret;
	int video_width = rk_param_get_int("video.2:width", -1);
	int video_height = rk_param_get_int("video.2:height", -1);
	int buf_cnt = 2;

	// VI
	VI_CHN_ATTR_S vi_chn_attr;
	memset(&vi_chn_attr, 0, sizeof(vi_chn_attr));
	if (enable_npu) // ensure vi and ivs have two buffer ping-pong
		buf_cnt += 1;
	vi_chn_attr.stIspOpt.u32BufCount = buf_cnt;
	vi_chn_attr.stIspOpt.enMemoryType = VI_V4L2_MEMORY_TYPE_DMABUF;
	vi_chn_attr.stIspOpt.stMaxSize.u32Width = rk_param_get_int("video.2:max_width", 960);
	vi_chn_attr.stIspOpt.stMaxSize.u32Height = rk_param_get_int("video.2:max_height", 540);
	vi_chn_attr.stSize.u32Width = video_width;
	vi_chn_attr.stSize.u32Height = video_height;
	vi_chn_attr.enPixelFormat = RK_FMT_YUV420SP;
	vi_chn_attr.enCompressMode = COMPRESS_MODE_NONE;
	vi_chn_attr.u32Depth = 0;
	if (enable_npu)
		vi_chn_attr.u32Depth += 1;
	ret = RK_MPI_VI_SetChnAttr(pipe_id_, VIDEO_PIPE_2, &vi_chn_attr);
	if (ret) {
		LOG_ERROR("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	ret = RK_MPI_VI_EnableChn(pipe_id_, VIDEO_PIPE_2);
	if (ret) {
		LOG_ERROR("ERROR: create VI error! ret=%d\n", ret);
		return ret;
	}

	if (enable_ivs) {
		rkipc_ivs_init();
		// bind
		vi_chn.enModId = RK_ID_VI;
		vi_chn.s32DevId = 0;
		vi_chn.s32ChnId = VIDEO_PIPE_2;
		ivs_chn.enModId = RK_ID_IVS;
		ivs_chn.s32DevId = 0;
		ivs_chn.s32ChnId = 0;
		ret = RK_MPI_SYS_Bind(&vi_chn, &ivs_chn);
		if (ret)
			LOG_ERROR("Bind VI and IVS error! ret=%#x\n", ret);
		else
			LOG_DEBUG("Bind VI and IVS success\n");
	}
	if (enable_npu) {
		pthread_create(&get_vi_2_send_thread, NULL, rkipc_get_vi_2_send, NULL);
		rkipc_osd_draw_nn_init();
	}
}

int rkipc_pipe_2_deinit() {
	int ret;
	if (enable_npu) {
		rkipc_osd_draw_nn_deinit();
		pthread_join(get_vi_2_send_thread, NULL);
	}
	if (enable_ivs) {
		// unbind
		vi_chn.enModId = RK_ID_VI;
		vi_chn.s32DevId = 0;
		vi_chn.s32ChnId = VIDEO_PIPE_2;
		ivs_chn.enModId = RK_ID_IVS;
		ivs_chn.s32DevId = 0;
		ivs_chn.s32ChnId = 0;
		ret = RK_MPI_SYS_UnBind(&vi_chn, &ivs_chn);
		if (ret)
			LOG_ERROR("Unbind VI and IVS error! ret=%#x\n", ret);
		else
			LOG_DEBUG("Unbind VI and IVS success\n");
		rkipc_ivs_deinit();
	}
	ret = RK_MPI_VI_DisableChn(pipe_id_, VIDEO_PIPE_2);
	if (ret)
		LOG_ERROR("ERROR: Destroy VI error! ret=%#x\n", ret);

	return 0;
}


int rkipc_pipe_jpeg_init() {
	// jpeg resolution same to video.0
	int ret;
	int video_width = rk_param_get_int("video.jpeg:width", 1920);
	int video_height = rk_param_get_int("video.jpeg:height", 1080);
	int video_max_height = rk_param_get_int("video.0:max_height", -1);
	int rotation = rk_param_get_int("video.source:rotation", 0);
	// VENC[3] init
	VENC_CHN_ATTR_S jpeg_chn_attr;
	memset(&jpeg_chn_attr, 0, sizeof(jpeg_chn_attr));
	jpeg_chn_attr.stVencAttr.enType = RK_VIDEO_ID_JPEG;
	jpeg_chn_attr.stVencAttr.enPixelFormat = RK_FMT_YUV420SP;
	jpeg_chn_attr.stVencAttr.u32MaxPicWidth = rk_param_get_int("video.0:max_width", 2560);
	jpeg_chn_attr.stVencAttr.u32MaxPicHeight = rk_param_get_int("video.0:max_height", 1440);
	jpeg_chn_attr.stVencAttr.u32PicWidth = video_width;
	jpeg_chn_attr.stVencAttr.u32PicHeight = video_height;
	jpeg_chn_attr.stVencAttr.u32VirWidth = video_width;
	jpeg_chn_attr.stVencAttr.u32VirHeight = video_height;
	jpeg_chn_attr.stVencAttr.u32StreamBufCnt = 2;
	jpeg_chn_attr.stVencAttr.u32BufSize = rk_param_get_int("video.jpeg:jpeg_buffer_size", 204800);
	// jpeg_chn_attr.stVencAttr.u32Depth = 1;
	ret = RK_MPI_VENC_CreateChn(JPEG_VENC_CHN, &jpeg_chn_attr);
	if (ret) {
		LOG_ERROR("ERROR: create VENC error! ret=%d\n", ret);
		return -1;
	}
	VENC_JPEG_PARAM_S stJpegParam;
	memset(&stJpegParam, 0, sizeof(stJpegParam));
	stJpegParam.u32Qfactor = rk_param_get_int("video.jpeg:jpeg_qfactor", 70);
	RK_MPI_VENC_SetJpegParam(JPEG_VENC_CHN, &stJpegParam);
	if (rotation == 0) {
		RK_MPI_VENC_SetChnRotation(JPEG_VENC_CHN, ROTATION_0);
	} else if (rotation == 90) {
		RK_MPI_VENC_SetChnRotation(JPEG_VENC_CHN, ROTATION_90);
	} else if (rotation == 180) {
		RK_MPI_VENC_SetChnRotation(JPEG_VENC_CHN, ROTATION_180);
	} else if (rotation == 270) {
		RK_MPI_VENC_SetChnRotation(JPEG_VENC_CHN, ROTATION_270);
	}

	VENC_CHN_BUF_WRAP_S stVencChnBufWrap;
	memset(&stVencChnBufWrap, 0, sizeof(stVencChnBufWrap));
	if (enable_wrap) {
		stVencChnBufWrap.bEnable = enable_wrap;
		stVencChnBufWrap.u32BufLine =
		    rk_param_get_int("video.source:buffer_line", video_max_height / 4);
		if (stVencChnBufWrap.u32BufLine < 128)
			stVencChnBufWrap.u32BufLine = video_max_height;
		RK_MPI_VENC_SetChnBufWrapAttr(JPEG_VENC_CHN, &stVencChnBufWrap);
	}

	VENC_COMBO_ATTR_S stComboAttr;
	memset(&stComboAttr, 0, sizeof(VENC_COMBO_ATTR_S));
	stComboAttr.bEnable = RK_TRUE;
	stComboAttr.s32ChnId = VIDEO_PIPE_0;
	RK_MPI_VENC_SetComboAttr(JPEG_VENC_CHN, &stComboAttr);

	VENC_RECV_PIC_PARAM_S stRecvParam;
	memset(&stRecvParam, 0, sizeof(VENC_RECV_PIC_PARAM_S));
	stRecvParam.s32RecvPicNum = 1;
	RK_MPI_VENC_StartRecvFrame(JPEG_VENC_CHN,
	                           &stRecvParam); // must, for no streams callback running failed

	pthread_create(&jpeg_venc_thread_id, NULL, rkipc_get_jpeg, NULL);
	if (rk_param_get_int("video.jpeg:enable_cycle_snapshot", 0)) {
		cycle_snapshot_flag = 1;
		pthread_create(&cycle_snapshot_thread_id, NULL, rkipc_cycle_snapshot, NULL);
	}

	return ret;
}

int rkipc_pipe_jpeg_deinit() {
	int ret = 0;
	ret = RK_MPI_VENC_StopRecvFrame(JPEG_VENC_CHN);
	ret |= RK_MPI_VENC_DestroyChn(JPEG_VENC_CHN);
	if (ret)
		LOG_ERROR("ERROR: Destroy VENC error! ret=%#x\n", ret);
	else
		LOG_INFO("RK_MPI_VENC_DestroyChn success\n");

	return ret;
}


int rkipc_osd_cover_create(int id, osd_data_s *osd_data) {
	LOG_DEBUG("id is %d\n", id);
	int ret = 0;
	RGN_HANDLE coverHandle = id;
	RGN_ATTR_S stCoverAttr;
	MPP_CHN_S stCoverChn;
	RGN_CHN_ATTR_S stCoverChnAttr;
	int rotation = rk_param_get_int("video.source:rotation", 0);
	int video_0_width = rk_param_get_int("video.0:width", -1);
	int video_0_height = rk_param_get_int("video.0:height", -1);
	int video_0_max_width = rk_param_get_int("video.0:max_width", -1);
	int video_0_max_height = rk_param_get_int("video.0:max_height", -1);
	double video_0_w_h_rate = 1.0;

	// since the coordinates stored in the OSD module are of actual resolution,
	// 1106 needs to be converted back to the maximum resolution
	osd_data->origin_x = osd_data->origin_x * video_0_max_width / video_0_width;
	osd_data->origin_y = osd_data->origin_y * video_0_max_height / video_0_height;
	osd_data->width = osd_data->width * video_0_max_width / video_0_width;
	osd_data->height = osd_data->height * video_0_max_height / video_0_height;

	memset(&stCoverAttr, 0, sizeof(stCoverAttr));
	memset(&stCoverChnAttr, 0, sizeof(stCoverChnAttr));
	// create cover regions
	stCoverAttr.enType = COVER_RGN;
	ret = RK_MPI_RGN_Create(coverHandle, &stCoverAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Create (%d) failed with %#x\n", coverHandle, ret);
		RK_MPI_RGN_Destroy(coverHandle);
		return RK_FAILURE;
	}
	LOG_DEBUG("The handle: %d, create success\n", coverHandle);

	// when cover is attached to VI,
	// coordinate conversion of three angles shall be considered when rotating VENC
	video_0_w_h_rate = (double)video_0_max_width / (double)video_0_max_height;
	if (rotation == 90) {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X =
		    (double)osd_data->origin_y * video_0_w_h_rate;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y =
		    (video_0_max_height -
		     ((double)(osd_data->width + osd_data->origin_x) / video_0_w_h_rate));
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width =
		    (double)osd_data->height * video_0_w_h_rate;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height =
		    (double)osd_data->width / video_0_w_h_rate;
	} else if (rotation == 270) {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X =
		    (video_0_max_width -
		     ((double)(osd_data->height + osd_data->origin_y) * video_0_w_h_rate));
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y =
		    (double)osd_data->origin_x / video_0_w_h_rate;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width =
		    (double)osd_data->height * video_0_w_h_rate;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height =
		    (double)osd_data->width / video_0_w_h_rate;
	} else if (rotation == 180) {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X =
		    video_0_max_width - osd_data->width - osd_data->origin_x;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y =
		    video_0_max_height - osd_data->height - osd_data->origin_y;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = osd_data->width;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = osd_data->height;
	} else {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X = osd_data->origin_x;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y = osd_data->origin_y;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width = osd_data->width;
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height = osd_data->height;
	}
	stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X =
	    UPALIGNTO16(stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X);
	stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y =
	    UPALIGNTO16(stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y);
	stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width =
	    UPALIGNTO16(stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width);
	stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height =
	    UPALIGNTO16(stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height);
	// because the rotation is done in the VENC,
	// and the cover and VI resolution are both before the rotation,
	// there is no need to judge the rotation here
	while (stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32X +
	           stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width >
	       video_0_max_width) {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Width -= 16;
	}
	while (stCoverChnAttr.unChnAttr.stCoverChn.stRect.s32Y +
	           stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height >
	       video_0_max_height) {
		stCoverChnAttr.unChnAttr.stCoverChn.stRect.u32Height -= 16;
	}

	// display cover regions to vi
	stCoverChn.enModId = RK_ID_VI;
	stCoverChn.s32DevId = 0;
	stCoverChn.s32ChnId = VI_MAX_CHN_NUM;
	stCoverChnAttr.bShow = osd_data->enable;
	stCoverChnAttr.enType = COVER_RGN;
	stCoverChnAttr.unChnAttr.stCoverChn.u32Color = 0xffffffff;
	stCoverChnAttr.unChnAttr.stCoverChn.u32Layer = id;
	LOG_DEBUG("cover region to chn success\n");
	ret = RK_MPI_RGN_AttachToChn(coverHandle, &stCoverChn, &stCoverChnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("vi pipe RK_MPI_RGN_AttachToChn (%d) failed with %#x\n", coverHandle, ret);
		return RK_FAILURE;
	}
	ret = RK_MPI_RGN_SetDisplayAttr(coverHandle, &stCoverChn, &stCoverChnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("vi pipe RK_MPI_RGN_SetDisplayAttr failed with %#x\n", ret);
		return RK_FAILURE;
	}
	LOG_DEBUG("RK_MPI_RGN_AttachToChn to vi 0 success\n");

	return ret;
}

int rkipc_osd_cover_destroy(int id) {
	LOG_DEBUG("%s\n", __func__);
	int ret = 0;
	// Detach osd from chn
	MPP_CHN_S stMppChn;
	RGN_HANDLE RgnHandle = id;

	stMppChn.enModId = RK_ID_VI;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = VI_MAX_CHN_NUM;
	ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
	if (RK_SUCCESS != ret)
		LOG_DEBUG("RK_MPI_RGN_DetachFrmChn (%d) to vi pipe failed with %#x\n", RgnHandle, ret);

	// destory region
	ret = RK_MPI_RGN_Destroy(RgnHandle);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Destroy [%d] failed with %#x\n", RgnHandle, ret);
	}
	LOG_DEBUG("Destory handle:%d success\n", RgnHandle);

	return ret;
}

int rkipc_osd_bmp_create(int id, osd_data_s *osd_data) {
	LOG_DEBUG("id is %d\n", id);
	int ret = 0;
	RGN_HANDLE RgnHandle = id;
	RGN_ATTR_S stRgnAttr;
	MPP_CHN_S stMppChn;
	RGN_CHN_ATTR_S stRgnChnAttr;
	BITMAP_S stBitmap;

	// create overlay regions
	memset(&stRgnAttr, 0, sizeof(stRgnAttr));
	stRgnAttr.enType = OVERLAY_RGN;
	stRgnAttr.unAttr.stOverlay.enPixelFmt = RK_FMT_ARGB8888;
	stRgnAttr.unAttr.stOverlay.u32CanvasNum = 2;
	stRgnAttr.unAttr.stOverlay.stSize.u32Width = osd_data->width;
	stRgnAttr.unAttr.stOverlay.stSize.u32Height = osd_data->height;
	ret = RK_MPI_RGN_Create(RgnHandle, &stRgnAttr);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Create (%d) failed with %#x\n", RgnHandle, ret);
		RK_MPI_RGN_Destroy(RgnHandle);
		return RK_FAILURE;
	}
	LOG_DEBUG("The handle: %d, create success\n", RgnHandle);

	// display overlay regions to venc groups
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	stMppChn.s32ChnId = 0;
	memset(&stRgnChnAttr, 0, sizeof(stRgnChnAttr));
	stRgnChnAttr.bShow = osd_data->enable;
	stRgnChnAttr.enType = OVERLAY_RGN;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X = osd_data->origin_x;
	stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y = osd_data->origin_y;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32BgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32FgAlpha = 128;
	stRgnChnAttr.unChnAttr.stOverlayChn.u32Layer = id;
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;

	// stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bEnable = true;
	// stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bForceIntra = false;
	// stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.bAbsQp = false;
	// stRgnChnAttr.unChnAttr.stOverlayChn.stQpInfo.s32Qp = -3;
	if (enable_venc_0) {
		stMppChn.s32ChnId = 0;
		ret = RK_MPI_RGN_AttachToChn(RgnHandle, &stMppChn, &stRgnChnAttr);
		if (RK_SUCCESS != ret) {
			LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) to venc0 failed with %#x\n", RgnHandle, ret);
			return RK_FAILURE;
		}
		LOG_DEBUG("RK_MPI_RGN_AttachToChn to venc0 success\n");
	}
	if (enable_venc_1) {
		stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32X =
		    UPALIGNTO16(osd_data->origin_x * rk_param_get_int("video.1:width", 1) /
		                rk_param_get_int("video.0:width", 1));
		stRgnChnAttr.unChnAttr.stOverlayChn.stPoint.s32Y =
		    UPALIGNTO16(osd_data->origin_y * rk_param_get_int("video.1:height", 1) /
		                rk_param_get_int("video.0:height", 1));
		stMppChn.s32ChnId = 1;
		ret = RK_MPI_RGN_AttachToChn(RgnHandle, &stMppChn, &stRgnChnAttr);
		if (RK_SUCCESS != ret) {
			LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) to venc1 failed with %#x\n", RgnHandle, ret);
			return RK_FAILURE;
		}
		LOG_DEBUG("RK_MPI_RGN_AttachToChn to venc1 success\n");
	}
	if (enable_jpeg) {
		stMppChn.s32ChnId = JPEG_VENC_CHN;
		ret = RK_MPI_RGN_AttachToChn(RgnHandle, &stMppChn, &stRgnChnAttr);
		if (RK_SUCCESS != ret) {
			LOG_ERROR("RK_MPI_RGN_AttachToChn (%d) to jpeg failed with %#x\n", RgnHandle, ret);
			return RK_FAILURE;
		}
		LOG_DEBUG("RK_MPI_RGN_AttachToChn to jpeg success\n");
	}

	// set bitmap
	stBitmap.enPixelFormat = RK_FMT_ARGB8888;
	stBitmap.u32Width = osd_data->width;
	stBitmap.u32Height = osd_data->height;
	stBitmap.pData = (RK_VOID *)osd_data->buffer;
	ret = RK_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
	if (ret != RK_SUCCESS) {
		LOG_ERROR("RK_MPI_RGN_SetBitMap failed with %#x\n", ret);
		return RK_FAILURE;
	}

	return ret;
}

int rkipc_osd_bmp_destroy(int id) {
	LOG_DEBUG("%s\n", __func__);
	int ret = 0;
	// Detach osd from chn
	MPP_CHN_S stMppChn;
	RGN_HANDLE RgnHandle = id;
	stMppChn.enModId = RK_ID_VENC;
	stMppChn.s32DevId = 0;
	if (enable_venc_0) {
		stMppChn.s32ChnId = 0;
		ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
		if (RK_SUCCESS != ret)
			LOG_DEBUG("RK_MPI_RGN_DetachFrmChn (%d) to venc0 failed with %#x\n", RgnHandle, ret);
	}
	if (enable_venc_1) {
		stMppChn.s32ChnId = 1;
		ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
		if (RK_SUCCESS != ret)
			LOG_DEBUG("RK_MPI_RGN_DetachFrmChn (%d) to venc1 failed with %#x\n", RgnHandle, ret);
	}
	if (enable_jpeg) {
		stMppChn.s32ChnId = JPEG_VENC_CHN;
		ret = RK_MPI_RGN_DetachFromChn(RgnHandle, &stMppChn);
		if (RK_SUCCESS != ret)
			LOG_DEBUG("RK_MPI_RGN_DetachFrmChn (%d) to jpeg failed with %#x\n", RgnHandle, ret);
	}

	// destory region
	ret = RK_MPI_RGN_Destroy(RgnHandle);
	if (RK_SUCCESS != ret) {
		LOG_ERROR("RK_MPI_RGN_Destroy [%d] failed with %#x\n", RgnHandle, ret);
	}
	LOG_DEBUG("Destory handle:%d success\n", RgnHandle);

	return ret;
}

int rkipc_osd_bmp_change(int id, osd_data_s *osd_data) {
	// LOG_DEBUG("id is %d\n", id);
	int ret = 0;
	RGN_HANDLE RgnHandle = id;
	BITMAP_S stBitmap;

	// set bitmap
	stBitmap.enPixelFormat = RK_FMT_ARGB8888;
	stBitmap.u32Width = osd_data->width;
	stBitmap.u32Height = osd_data->height;
	stBitmap.pData = (RK_VOID *)osd_data->buffer;
	ret = RK_MPI_RGN_SetBitMap(RgnHandle, &stBitmap);
	if (ret != RK_SUCCESS) {
		LOG_ERROR("RK_MPI_RGN_SetBitMap failed with %#x\n", ret);
		return RK_FAILURE;
	}

	return ret;
}

int rkipc_osd_init() {
	rk_osd_cover_create_callback_register(rkipc_osd_cover_create);
	rk_osd_cover_destroy_callback_register(rkipc_osd_cover_destroy);
	rk_osd_bmp_create_callback_register(rkipc_osd_bmp_create);
	rk_osd_bmp_destroy_callback_register(rkipc_osd_bmp_destroy);
	rk_osd_bmp_change_callback_register(rkipc_osd_bmp_change);
	rk_osd_init();

	return 0;
}

int rkipc_osd_deinit() {
	rk_osd_deinit();
	rk_osd_cover_create_callback_register(NULL);
	rk_osd_cover_destroy_callback_register(NULL);
	rk_osd_bmp_create_callback_register(NULL);
	rk_osd_bmp_destroy_callback_register(NULL);
	rk_osd_bmp_change_callback_register(NULL);

	return 0;
}



int rk_video_init()
{
	LOG_DEBUG("begin\n");
	int ret = 0;
	enable_ivs = rk_param_get_int("video.source:enable_ivs", 1);
	enable_jpeg = rk_param_get_int("video.source:enable_jpeg", 1);
	enable_venc_0 = rk_param_get_int("video.source:enable_venc_0", 1);
	enable_venc_1 = rk_param_get_int("video.source:enable_venc_1", 1);
	enable_rtsp = rk_param_get_int("video.source:enable_rtsp", 1);
	LOG_INFO("enable_jpeg is %d, enable_venc_0 is %d, enable_venc_1 is %d, enable_rtsp is %d", enable_jpeg, enable_venc_0, enable_venc_1, enable_rtsp);

	g_vi_chn_id = rk_param_get_int("video.source:vi_chn_id", 0);
	enable_npu = rk_param_get_int("video.source:enable_npu", 0);
	enable_wrap = rk_param_get_int("video.source:enable_wrap", 0);
	enable_osd = rk_param_get_int("osd.common:enable_osd", 0);
	LOG_DEBUG("g_vi_chn_id is %d, enable_npu is %d, "
	          "enable_wrap is %d, enable_osd is %d\n",
	          g_vi_chn_id, enable_npu, enable_wrap, enable_osd);
	g_video_run_ = 1;
	g_nn_osd_run_ = 1;
	ret |= rkipc_vi_dev_init();
	if (enable_rtsp)
		ret |= rkipc_rtsp_init();
	if (enable_venc_0)
		ret |= rkipc_pipe_0_init();
	if (enable_venc_1)
		ret |= rkipc_pipe_1_init();
	if (enable_jpeg)
		ret |= rkipc_pipe_jpeg_init();

	if (enable_npu || enable_ivs) {
		ret |= rkipc_pipe_2_init();
	}
	// The osd dma buffer must be placed in the last application,
	// otherwise, when the font size is switched, holes may be caused
	if (enable_osd)
		ret |= rkipc_osd_init();
	LOG_DEBUG("over\n");

	return ret;
}

int rk_video_deinit() {
	LOG_DEBUG("%s\n", __func__);
	g_video_run_ = 0;
	g_nn_osd_run_ = 0;
	int ret = 0;
	if (enable_npu || enable_ivs)
		ret |= rkipc_pipe_2_deinit();

	if (enable_osd)
		ret |= rkipc_osd_deinit();

	if (enable_venc_0) {
		pthread_join(venc_thread_0, NULL);
		ret |= rkipc_pipe_0_deinit();
	}
	if (enable_venc_1) {
		pthread_join(venc_thread_1, NULL);
		ret |= rkipc_pipe_1_deinit();
	}
	if (enable_jpeg) {
		if (rk_param_get_int("video.jpeg:enable_cycle_snapshot", 0)) {
			cycle_snapshot_flag = 0;
			pthread_join(cycle_snapshot_thread_id, NULL);
		}
		pthread_join(jpeg_venc_thread_id, NULL);
		ret |= rkipc_pipe_jpeg_deinit();
	}
	ret |= rkipc_vi_dev_deinit();

	if (enable_rtsp)
		ret |= rkipc_rtsp_deinit();

	return ret;
}

extern char *rkipc_iq_file_path_;
int rk_video_restart() {
	int ret;
	ret = rk_storage_deinit();
	ret |= rk_video_deinit();
	if (rk_param_get_int("video.source:enable_aiq", 1))
		ret |= rk_isp_deinit(0);
	if (rk_param_get_int("video.source:enable_aiq", 1)) {
		ret |= rk_isp_init(0, rkipc_iq_file_path_);
		if (rk_param_get_int("isp:init_form_ini", 1))
			ret |= rk_isp_set_from_ini(0);
	}
	ret |= rk_video_init();
	if (rk_param_get_int("audio.0:enable", 0))
		rkipc_audio_rtsp_init();
	ret |= rk_storage_init();

	return ret;
}