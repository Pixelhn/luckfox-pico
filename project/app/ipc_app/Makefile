CROSS_COMPILE=arm-rockchip830-linux-uclibcgnueabihf
CC=${CROSS_COMPILE}-gcc
STRIP=${CROSS_COMPILE}-strip

BUILD_DIR=build
SDK_DIR=../../..
TARGET=rkipc

CFLAGS+=

LDFLAGS+=  
#-static

#main
C_SOURCES += src/rv1103_ipc/main.c \
	src/rv1103_ipc/video/video.c \
	src/rv1103_ipc/audio/audio.c

C_INCLUDES += -Isrc/rv1103_ipc/audio \
	-Isrc/rv1103_ipc/video

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
	common/storage/storage.c \
	common/network/network.c \
	common/network/Rk_wifi.c \
	common/network/utility.c \
	common/network/RK_encode.c \
	common/network/utf8_to_gbk.c \
	common/network/gbk_to_utf8.c \
	common/network/Hostapd.c \
	common/network/ntp.c \
	common/system/system.c \
	common/event/event.c \
	common/rtmp/rtmp.c \
	common/region_clip/region_clip.c \
	common/roi/roi.c \

C_INCLUDES += -Icommon \
	-Icommon/osd \
	-Icommon/param \
	-Icommon/rockiva \
	-Icommon/storage \
	-Icommon/network \
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
	-I${SDK_DIR}/media/sysutils/include \
	-I$(SDK_DIR)/media/libiconv/out/include \
	-I$(SDK_DIR)/project/app/component/third_libs/include/\
	-I$(SDK_DIR)/project/app/component/third_libs/out/include/freetype2/ \
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
	-lrtsp -lrkmuxer -L${SDK_DIR}/media/common_algorithm/common_algorithm/misc/lib/arm-rockchip830-linux-uclibcgnueabihf \
	-lrockiva 		-L${SDK_DIR}/media/iva/iva/librockiva/rockiva-rv1106-Linux/lib \
	-liconv 		-L$(SDK_DIR)/project/app/component/third_libs/lib/arm-rockchip830-linux-uclibcgnueabihf \
	-lfreetype 		-L$(SDK_DIR)/project/app/component/third_libs/lib/arm-rockchip830-linux-uclibcgnueabihf \
	-lz 			-L$(SDK_DIR)/sysdrv/tools/board/toolkits/zlib/out/lib/ \
	-lwpa_client -Llib/arm-rockchip830-linux-uclibcgnueabihf/ \

OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR) 
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR):
	mkdir $@

clean:
	-rm -rf ${BUILD_DIR}