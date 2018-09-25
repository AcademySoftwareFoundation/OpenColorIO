/*
Copyright (c) 2003-2010 Sony Pictures Imageworks Inc., et al.
All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:
* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
* Neither the name of Sony Pictures Imageworks nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <string>
#include <sstream>
#include <vector>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
OCIO_NAMESPACE_USING

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
