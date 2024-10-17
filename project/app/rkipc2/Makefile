ifeq ($(APP_PARAM), )
    APP_PARAM:=../Makefile.param
    include $(APP_PARAM)
endif

export LC_ALL=C
SHELL:=/bin/bash

CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf
CC=${CROSS_COMPILE}-gcc
STRIP=${CROSS_COMPILE}-strip
CPP=${CROSS_COMPILE}-g++

BUILD_DIR=build
SDK_DIR=../../..
TARGET=rkipc

CFLAGS+= -D__LINUX__

LDFLAGS+=  
#-static

#main
ifeq ($(RK_APP_TYPE), RKIPC_RV1103)

C_SOURCES += src/rv1103_ipc/main.c \
	src/rv1103_ipc/video/video.c \
	# src/rv1103_ipc/audio/audio.c

C_INCLUDES += -Isrc/rv1103_ipc/audio \
	-Isrc/rv1103_ipc/video

else ifeq ($(RK_APP_TYPE), RKIPC_RV1106)

C_SOURCES += src/rv1106_ipc/main.c \
	src/rv1106_ipc/video/video.c \
	# src/rv1103_ipc/audio/audio.c

C_INCLUDES += -Isrc/rv1106_ipc/audio \
	-Isrc/rv1106_ipc/video

endif


#isp
C_SOURCES += common/isp/rv1106/isp.c

C_INCLUDES += -Icommon/isp/rv1106/

#common
C_SOURCES +=  common/common.c \
	common/osd/osd.c \
	common/osd/font_factory.c \
	common/osd/bmp_reader.c \
	common/param/param.c \
	common/param/iniparser.c \
	common/param/dictionary.c \
	common/rockiva/rockiva.c \
	common/rtsp_demo/rtp_enc.c \
	common/rtsp_demo/rtsp_demo.c \
	common/rtsp_demo/stream_queue.c \
	common/rtsp_demo/utils.c \
	common/rtsp_demo/rtsp_msg.c

#	common/storage/storage.c \
	common/network/network.c \
	common/network/Rk_wifi.c \
	common/network/utility.c \
	common/network/RK_encode.c \
	common/network/utf8_to_gbk.c \
	common/network/gbk_to_utf8.c \
	common/network/Hostapd.c \
	common/network/ntp.c \
	common/socket_server/server.c \
	common/socket_server/socket.c \
	common/system/system.c \
	common/event/event.c \
	common/rtmp/rtmp.c \
	common/socket_server/server.c \
	common/socket_server/socket.c \
	common/region_clip/region_clip.c \
	common/roi/roi.c \

C_INCLUDES += -Icommon \
	-Icommon/osd \
	-Icommon/param \
	-Icommon/rockiva \
	-Icommon/storage \
#	-Icommon/network \
	-Icommon/system \
	-Icommon/roi \
	-Icommon/rtmp \
	-Icommon/event \
	-Icommon/socket_server \
	-Icommon/region_clip \

#lib
C_INCLUDES += -I${SDK_DIR}/media/iva/out/include \
	-I${SDK_DIR}/media/common_algorithm/common_algorithm/misc/include \
	-I${SDK_DIR}/media/rockit/rockit/mpi/sdk/include \
	-I${SDK_DIR}/media/rga/release_rga_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include \
	-I${SDK_DIR}/media/iva/iva/librockiva/rockiva-rv1106-Linux/include/ \
	-I${SDK_DIR}/media/sysutils/include \
	-I$(SDK_DIR)/media/libiconv/out/include \
	-I$(SDK_DIR)/media/freetype/out/include \
	-I$(SDK_DIR)/media/freetype/out/include/freetype2 \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/uAPI2 \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/algos \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/xcore \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/common \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/iq_parser_v2 \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq \
	-I${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/include/rkaiq/iq_parser \
	-I$(SDK_DIR)/sysdrv/tools/board/toolkits/zlib/out/include

CFLAGS+=${C_INCLUDES}

LDFLAGS+= \
	-lrkaiq 		-L${SDK_DIR}/media/isp/release_camera_engine_rkaiq_rv1106_arm-rockchip830-linux-uclibcgnueabihf/lib \
	-lrockit 		-L${SDK_DIR}/media/rockit/rockit/lib/lib32 \
	-lrockchip_mpp 	-L${SDK_DIR}/media/mpp/release_mpp_rv1106_arm-rockchip830-linux-uclibcgnueabihf/lib \
	-lrga 			-L${SDK_DIR}/media/rga/release_rga_rv1106_arm-rockchip830-linux-uclibcgnueabihf/lib \
	-lrksysutils 	-L${SDK_DIR}/media/sysutils/out/lib \
	-lrockiva 		-L${SDK_DIR}/media/iva/iva/librockiva/rockiva-rv1106-Linux/lib \
	-liconv 		-L$(SDK_DIR)/media/libiconv/out/lib \
	-lfreetype 		-L$(SDK_DIR)/media/freetype/out/lib \
	-lz 			-L$(SDK_DIR)/sysdrv/tools/board/toolkits/zlib/out/lib/
#-lwpa_client -Llib/arm-rockchip830-linux-uclibcgnueabihf/ \
#-lrtsp -lrkmuxer -L${SDK_DIR}/media/common_algorithm/common_algorithm/misc/lib/arm-rockchip830-linux-uclibcgnueabihf \

CPP_SOURCE+=

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

CPP_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCE:.cpp=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCE)))

all: $(BUILD_DIR)/$(TARGET) install

install:
	test -z "$(RK_PROJECT_PATH_APP)" || ( \
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_PROJECT_PATH_APP)/bin, $(BUILD_DIR)/$(TARGET)); \
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_PROJECT_PATH_APP)/bin, $(BUILD_DIR)/../src/RkLunch.sh); \
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_PROJECT_PATH_APP)/bin, $(BUILD_DIR)/../src/RkLunch-stop.sh); \
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_PROJECT_PATH_APP)/share, $(BUILD_DIR)/../src/rkipc-300w.ini); \
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_PROJECT_PATH_APP)/share, $(BUILD_DIR)/../common/osd/simsun_en.ttf); \
	)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR) 
	$(CPP) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS) $(CPP_OBJECTS) Makefile
	$(CC) $(OBJECTS) $(CPP_OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir $@

clean:
	-rm -rf ${BUILD_DIR}