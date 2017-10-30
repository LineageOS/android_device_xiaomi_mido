ifneq ($(QCPATH),)
ifneq ($(BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE),)

LOCAL_PATH := $(call my-dir)

#add QMI libraries for QMI targets
QMI_BOARD_PLATFORM_LIST := msm8960
QMI_BOARD_PLATFORM_LIST += msm8974
QMI_BOARD_PLATFORM_LIST += msm8226
QMI_BOARD_PLATFORM_LIST += msm8610
QMI_BOARD_PLATFORM_LIST += apq8084
QMI_BOARD_PLATFORM_LIST += msm8916
QMI_BOARD_PLATFORM_LIST += msm8994
QMI_BOARD_PLATFORM_LIST += msm8909
QMI_BOARD_PLATFORM_LIST += msm8952
QMI_BOARD_PLATFORM_LIST += msm8992
QMI_BOARD_PLATFORM_LIST += msm8996
QMI_BOARD_PLATFORM_LIST += msm8953
QMI_BOARD_PLATFORM_LIST += msm8937
QMI_BOARD_PLATFORM_LIST += msm8998
QMI_BOARD_PLATFORM_LIST += apq8098_latv
QMI_BOARD_PLATFORM_LIST += sdm660
QMI_BOARD_PLATFORM_LIST += sdm845
QMI_BOARD_PLATFORM_LIST += msmpeafowl

ifneq (,$(filter $(QMI_BOARD_PLATFORM_LIST),$(TARGET_BOARD_PLATFORM)))
include $(call all-subdir-makefiles)
endif #is-board-platform-in-list

# link gss.bxx files into /etc/firmware folder
ifeq ($(call is-board-platform-in-list, msm8960),true)
$(shell ln -sf /firmware/image/gss.b00 $(TARGET_OUT_ETC)/firmware/gss.b00)
$(shell ln -sf /firmware/image/gss.b01 $(TARGET_OUT_ETC)/firmware/gss.b01)
$(shell ln -sf /firmware/image/gss.b02 $(TARGET_OUT_ETC)/firmware/gss.b02)
$(shell ln -sf /firmware/image/gss.b03 $(TARGET_OUT_ETC)/firmware/gss.b03)
$(shell ln -sf /firmware/image/gss.b04 $(TARGET_OUT_ETC)/firmware/gss.b04)
$(shell ln -sf /firmware/image/gss.b05 $(TARGET_OUT_ETC)/firmware/gss.b05)
$(shell ln -sf /firmware/image/gss.b06 $(TARGET_OUT_ETC)/firmware/gss.b06)
$(shell ln -sf /firmware/image/gss.b07 $(TARGET_OUT_ETC)/firmware/gss.b07)
$(shell ln -sf /firmware/image/gss.b08 $(TARGET_OUT_ETC)/firmware/gss.b08)
$(shell ln -sf /firmware/image/gss.b09 $(TARGET_OUT_ETC)/firmware/gss.b09)
$(shell ln -sf /firmware/image/gss.b10 $(TARGET_OUT_ETC)/firmware/gss.b10)
$(shell ln -sf /firmware/image/gss.b11 $(TARGET_OUT_ETC)/firmware/gss.b11)
$(shell ln -sf /firmware/image/gss.b12 $(TARGET_OUT_ETC)/firmware/gss.b12)
$(shell ln -sf /firmware/image/gss.b13 $(TARGET_OUT_ETC)/firmware/gss.b13)
$(shell ln -sf /firmware/image/gss.b14 $(TARGET_OUT_ETC)/firmware/gss.b14)
$(shell ln -sf /firmware/image/gss.b15 $(TARGET_OUT_ETC)/firmware/gss.b15)
$(shell ln -sf /firmware/image/gss.mdt $(TARGET_OUT_ETC)/firmware/gss.mdt)
endif

endif#BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE
endif#QCPATH
