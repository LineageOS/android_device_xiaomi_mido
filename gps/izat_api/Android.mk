ifneq ($(BUILD_TINY_ANDROID),true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := izat_remote_api_headers
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)
include $(BUILD_HEADER_LIBRARY)

endif # not BUILD_TINY_ANDROID
