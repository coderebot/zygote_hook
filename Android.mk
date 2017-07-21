
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

enable_debug64:=false
enable_debug32:=false


LOCAL_C_INCLUDES :=  \
        $(JNI_H_INCLUDE) \

ifneq ($(shell expr $(PLATFORM_SDK_VERSION) \>= 21),0)
include art/build/Android.common_build.mk
LOCAL_C_INCLUDES += external/libcxx/include
ART_UTILS_SHARED_LIBS += libc++
ART_UTILS_SHARED_LIBS += libart
LOCAL_CFLAGS += $(MTK_CFLAGS)
else
include art/build/Android.common.mk
LOCAL_C_INCLUDES += external/stlport/stlport \
        bionic \
        bionic/libstdc++/include
LOCAL_CFLAGS += -DANDROID_SMP=1
endif

LOCAL_C_INCLUDES += art/runtime \
            $(ART_C_INCLUDES)

SOURCE_FLAGS:= -DPLATFORM_SDK_VERSION=$(PLATFORM_SDK_VERSION)

ifeq (true,$(enable_debug64))
SOURCE_FLAGS += -DENABLE_DEBUG64
endif

ifeq (true,$(enable_debug32))
SOURCE_FLAGS += -DENABLE_DEBUG32
endif

ASM_SRC_FILES:= \
	hook-arm64.S \
	hook-arm32.S

LOCAL_SRC_FILES := \
	main.cpp \
	$(ASM_SRC_FILES) \
	stub.cpp \
	init.cpp \
	user-callback.cpp

LOCAL_SHARED_LIBRARIES := liblog  $(ART_UTILS_SHARED_LIBS)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libzygotehook
LOCAL_CLANG := true
LOCAL_CFLAGS := -Werror $(SOURCE_FLAGS) $(MTK_CFLAGS) \
	-DIMT_SIZE=64 \
    $(ART_TARGET_CFLAGS)

LOCAL_ASFLAGS:= -Werror $(SOURCE_FLAGS) $(MTK_CFLAGS)

#LOCAL_ADDITIONAL_DEPENDENCIES := $(LOCAL_PATH)/Android.mk

include $(BUILD_SHARED_LIBRARY)

