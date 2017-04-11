LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE        := libplayer_jni
LOCAL_CFLAGS        := -Werror

LOCAL_C_INCLUDES    :=  $(LOCAL_PATH)
LOCAL_SRC_FILES     :=  file/file.cpp               \
                        security/aes.cpp            \
                        security/licence.cpp        \
                        transform/touch.cpp         \
                        transform/sensor.cpp        \
                        transform/transform.cpp     \
                        gl/gl_matrix.cpp            \
                        gl/gl_base.cpp              \
                        gl/gl_renderer.cpp          \
                        gl/gl_picture.cpp           \
                        gl/gl_video.cpp             \
                        bean/float_buffer.cpp       \
                        bean/bean.cpp               \
                        bean/region.cpp             \
                        jni_api.cpp

LOCAL_LDLIBS        := -llog -lGLESv3 -lEGL -ljnigraphics

include $(BUILD_SHARED_LIBRARY)