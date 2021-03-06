#include "gl/gl_yuv.h"

PictureYuv::PictureYuv(TransformBean *transformBean, SettingsBean *settingsBean)
        : GLRenderer(transformBean, settingsBean) {
    LOGI("[PictureYuv] +");
    bFirstFrame = GL_TRUE;
    mSettingsBean = settingsBean;

    pthread_mutex_init(&mutex, NULL);
    pSO = dlopen("/data/data/com.wq.playerdemo/lib/libijkplayer.so", RTLD_NOW);
    if (pSO == NULL) {
        LOGE("[jin_api]load libijkplayer.so from lib  fail!");
        pSO = dlopen("/data/data/com.wq.playerdemo/lib64/libijkplayer.so", RTLD_NOW);
        if (pSO == NULL) {
            LOGE("[jin_api]load libijkplayer.so from lib64  fail!");
            exit(0);
        }
    }
    funcGetFrame = (AVFrame *(*)()) dlsym(pSO, "ijkmp_get_frame");
}

PictureYuv::~PictureYuv() {
    LOGI("[PictureYuv] -");

    pthread_mutex_destroy(&mutex);
}

void PictureYuv::loadShader() {
    GLRenderer::loadShader();
    pBeanProcess->mProgramHandle = createProgram(gYuvVertexShader, gYuvFragmentShader);
    // 获取投影、Camera、变换句柄
    pBeanProcess->mProjectionHandle = glGetUniformLocation(pBeanProcess->mProgramHandle,
                                                           "projection");
    pBeanProcess->mCameraHandle = glGetUniformLocation(pBeanProcess->mProgramHandle,
                                                       "camera");
    pBeanProcess->mTransformHandle = glGetUniformLocation(pBeanProcess->mProgramHandle,
                                                          "transform");
    pBeanProcess->mLightHandle = glGetUniformLocation(pBeanProcess->mProgramHandle,
                                                      "light");
    mTexHandleY = glGetUniformLocation(pBeanProcess->mProgramHandle, "tex_y");
    mTexHandleU = glGetUniformLocation(pBeanProcess->mProgramHandle, "tex_u");
    mTexHandleV = glGetUniformLocation(pBeanProcess->mProgramHandle, "tex_v");
    LOGI("[PictureYuv:loadShader]pBeanProcess->mProgramHandle=%d", pBeanProcess->mProgramHandle);
}

GLboolean PictureYuv::prepareDraw(Bitmap *bmp) {
    pFrame = funcGetFrame();
    if (pFrame != NULL) {
        if (bFirstFrame) {
            bFirstFrame = GL_FALSE;
            createTextureForYUV();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextureY);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                         pFrame->width, pFrame->height, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         pFrame->data[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextureU);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                         pFrame->width / 2, pFrame->height / 2, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         pFrame->data[1]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mTextureV);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE,
                         pFrame->width / 2, pFrame->height / 2, 0,
                         GL_LUMINANCE, GL_UNSIGNED_BYTE,
                         pFrame->data[2]);

        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextureY);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            pFrame->width, pFrame->height,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            pFrame->data[0]);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextureU);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            pFrame->width / 2, pFrame->height / 2,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            pFrame->data[1]);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mTextureV);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                            pFrame->width / 2, pFrame->height / 2,
                            GL_LUMINANCE, GL_UNSIGNED_BYTE,
                            pFrame->data[2]);
        }
        return GL_TRUE;
    } else {
        return GL_FALSE;
    }
}

GLuint PictureYuv::updateTextureAuto() {
    return 10;
}

void PictureYuv::drawForYUV(GLBean *glBean) {
    glUseProgram(glBean->mProgramHandle);
    for (GLuint i = 0; i < glBean->pVertexBuffer->getSize(); i++) {
        // 绑定VAO，绑定之后开始绘制
        glBindVertexArray(glBean->pVAO[i]);
        checkGLError("draw glBindVertexArray +");

        // 投影、Camera、变换赋值
        if (glBean->pTransformBean != NULL) {
            glBean->pMatrix->perspective(glBean->pTransformBean->fov,
                                         (GLfloat) mWindowWidth / (GLfloat) mWindowHeight,
                                         0.1,
                                         100);
            glBean->pMatrix->setIdentity();
            glBean->pMatrix->rotate(glBean->pTransformBean->degreeY, 1, 0, 0);
            glBean->pMatrix->rotate(glBean->pTransformBean->degreeX, 0, 1, 0);
        }
        if (glBean->mProjectionHandle != -1) {
            glUniformMatrix4fv(glBean->mProjectionHandle, 1, GL_FALSE,
                               glBean->pMatrix->getProjectionMatrix());
        }
        if (glBean->mCameraHandle != -1) {
            glUniformMatrix4fv(glBean->mCameraHandle, 1, GL_FALSE,
                               glBean->pMatrix->getCameraMatrix());
        }
        if (glBean->mTransformHandle != -1) {
            glUniformMatrix4fv(glBean->mTransformHandle, 1, GL_FALSE,
                               glBean->pMatrix->getTransformMatrix());
        }
        if (glBean->mLightHandle != -1) {
            glUniform3f(glBean->mLightHandle, 1, 1, 1);
        }
        if (glBean->mComposeTextureId != 0 && i > 0) {
            glBindTexture(glBean->eTextureTarget, glBean->mComposeTextureId);
        } else {
            glBindTexture(glBean->eTextureTarget, glBean->mTextureId);
            // bind textures
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mTextureY);
            glUniform1i(mTexHandleY, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, mTextureU);
            glUniform1i(mTexHandleU, 1);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, mTextureV);
            glUniform1i(mTexHandleV, 2);
        }
        glDrawArrays(GL_TRIANGLE_STRIP, 0, glBean->pVertexBuffer->getBuffer(i)->pointSize);
        // 解绑VAO
        glBindVertexArray(0);
        checkGLError("draw glBindVertexArray -");
    }
}

GLboolean PictureYuv::useYUVDraw() {
    return GL_TRUE;
}

void PictureYuv::createTextureForYUV() {
    glGenTextures(1, &mTextureY);
//    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mTextureY);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &mTextureU);
//    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mTextureU);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &mTextureV);
//    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mTextureV);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}