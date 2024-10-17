export SYSDRV_ARCH_TYPE=arm
export SYSDRV_CROSS=arm-rockchip830-linux-uclibcgnueabihf

export LC_ALL=C
export SHELL:=/bin/bash
CURRENT_DIR := $(shell pwd)

BUILD_TARGET := ${CURRENT_DIR}/nginx-1.26.2.tar.gz
BUILD_TARGET_DIR := ${CURRENT_DIR}/nginx-1.26.2
BUILD_TARGET_OUT := ${CURRENT_DIR}/out
INSTALL_TARGET := ${BUILD_TARGET_OUT}/sbin/nginx

LIB_OPENSSL=${CURRENT_DIR}/../toolkits/openssl/
LIB_PCRE2=${CURRENT_DIR}/../pcre2/
LIB_ZLIB=${CURRENT_DIR}/../toolkits/zlib/

all:lib_openssl lib_pcre2 lib_zlib
	@test -f $(INSTALL_TARGET) || ( \
		rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}; \
		tar -xf ${BUILD_TARGET}; \
		mkdir -p $(BUILD_TARGET_OUT); \
		pushd $(BUILD_TARGET_DIR)/; \
			patch -p1 < ../configure.patch;	\
			./configure --prefix=${BUILD_TARGET_OUT} \
			--with-http_ssl_module \
			--with-cc=${SYSDRV_CROSS}-gcc \
 			--with-cpp=${SYSDRV_CROSS}-g++ \
 			--with-cc-opt="-I ${LIB_PCRE2}/out/include -I ${LIB_OPENSSL}/out3/include -I ${LIB_ZLIB}/out/include" \
 			--with-ld-opt="-L ${LIB_PCRE2}/out/lib -L ${LIB_OPENSSL}/out3/lib -L ${LIB_ZLIB}/out/lib" \
 			\
			--without-http_upstream_zone_module \
			--without-http_fastcgi_module \
			--without-http_uwsgi_module \
			--without-http_scgi_module \
			\
			--with-http_gzip_static_module \
			\
			--http-proxy-temp-path=/tmp/nginx/proxy_temp \
			--http-client-body-temp-path=/tmp/nginx/client_body_temp \
			\
			--lock-path=/tmp/nginx/nginx.lock \
			--pid-path=/tmp/nginx.pid \
			\
			--error-log-path=/tmp/nginx/error.log \
			--http-log-path=/tmp/nginx/access.log; \
			patch -p1 < ../make.patch; \
			make -j$(SYSDRV_JOBS); \
			make install; \
		popd;)
	test -z "$(SYSDRV_DIR_OUT_ROOTFS)" || ( \
		cp -rfp $(BUILD_TARGET_OUT)/sbin/nginx  $(SYSDRV_DIR_OUT_ROOTFS)/usr/bin; \
		cp -rfp $(BUILD_TARGET_OUT)/../nginx.sh  $(SYSDRV_DIR_OUT_ROOTFS)/etc/profile.d/nginx.sh; \
		\
		mkdir -p $(SYSDRV_DIR_OUT_ROOTFS)/root/web/conf/;\
		mkdir -p $(SYSDRV_DIR_OUT_ROOTFS)/root/web/html/;\
		cp -rfp $(BUILD_TARGET_OUT)/conf/*  $(SYSDRV_DIR_OUT_ROOTFS)/root/web/conf/; \
		cp -rfp $(BUILD_TARGET_OUT)/html/*  $(SYSDRV_DIR_OUT_ROOTFS)/root/web/html/; \
	)

lib_openssl:
	make -C ${LIB_OPENSSL}

lib_pcre2:
	make -C ${LIB_PCRE2}

lib_zlib:
	make -C ${LIB_ZLIB}

clean: distclean

distclean:
	-rm -rf ${BUILD_TARGET_DIR} ${BUILD_TARGET_OUT}


# --http-fastcgi-temp-path=/tmp/nginxx \
# --http-proxy-temp-path=/tmp/nginxx \
# --http-uwsgi-temp-path=/tmp/nginxx \
# --http-client-body-temp-path=/tmp/nginxx \
# --http-scgi-temp-path=/tmp/nginxx \