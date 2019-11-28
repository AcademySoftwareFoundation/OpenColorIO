// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>
#include <sstream>
#include <vector>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Processor_Create(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    jobject obj = BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
        env->FindClass("org/OpenColorIO/Processor"), Processor::Create());
    return obj;
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Processor_isNoOp(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    return (jboolean)ptr->isNoOp();
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Processor_hasChannelCrosstalk(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    return (jboolean)ptr->hasChannelCrosstalk();
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Processor_apply(JNIEnv * env, jobject self, jobject img) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ImageDescRcPtr _img = GetEditableJOCIO<ImageDescRcPtr, ImageDescJNI>(env, img);
    ptr->apply(*_img.get());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Processor_applyRGB(JNIEnv * env, jobject self, jfloatArray pixel) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ptr->applyRGB(GetJFloatArrayValue(env, pixel, "pixel", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Processor_applyRGBA(JNIEnv * env, jobject self, jfloatArray pixel) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ptr->applyRGBA(GetJFloatArrayValue(env, pixel, "pixel", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Processor_getCpuCacheID(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    return env->NewStringUTF(ptr->getCpuCacheID());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Processor_getGpuShaderText(JNIEnv * env, jobject self, jobject shaderDesc) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ConstGpuShaderDescRcPtr desc = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, shaderDesc);
    return env->NewStringUTF(ptr->getGpuShaderText(*desc.get()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Processor_getGpuShaderTextCacheID(JNIEnv * env, jobject self, jobject shaderDesc) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ConstGpuShaderDescRcPtr desc = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, shaderDesc);
    return env->NewStringUTF(ptr->getGpuShaderTextCacheID(*desc.get()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Processor_getGpuLut3D(JNIEnv * env, jobject self, jobject lut3d, jobject shaderDesc) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ConstGpuShaderDescRcPtr desc = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, shaderDesc);
    int len = desc->getLut3DEdgeLen();
    int size = 3*len*len*len;
    float* _lut3d = GetJFloatBuffer(env, lut3d, size);
    ptr->getGpuLut3D(_lut3d, *desc.get());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Processor_getGpuLut3DCacheID(JNIEnv * env, jobject self, jobject shaderDesc) {
    OCIO_JNITRY_ENTER()
    ConstProcessorRcPtr ptr = GetConstJOCIO<ConstProcessorRcPtr, ProcessorJNI>(env, self);
    ConstGpuShaderDescRcPtr desc = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, shaderDesc);
    return env->NewStringUTF(ptr->getGpuLut3DCacheID(*desc.get()));
    OCIO_JNITRY_EXIT(NULL)
}
