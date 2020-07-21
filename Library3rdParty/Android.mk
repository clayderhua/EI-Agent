
#include $(call all-subdir-makefiles)
include $(filter-out $(call my-dir)/AdvLog/Android.mk,$(call all-subdir-makefiles))
