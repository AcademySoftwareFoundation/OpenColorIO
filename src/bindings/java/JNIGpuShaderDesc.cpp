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

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
OCIO_NAMESPACE_USING

namespace
{

void GpuShaderDesc_deleter(GpuShaderDesc* d)
{
    delete d;
}

}; // end anon namespace

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GpuShaderDesc_create(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    GpuShaderDescJNI * jnistruct = new GpuShaderDescJNI();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new ConstGpuShaderDescRcPtr();
    jnistruct->cppobj = new GpuShaderDescRcPtr();
    *jnistruct->cppobj = GpuShaderDescRcPtr(new GpuShaderDesc(), &GpuShaderDesc_deleter);
    jnistruct->isconst = false;
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    env->SetLongField(self, fid, (jlong)jnistruct);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GpuShaderDesc_dispose(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<GpuShaderDescJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GpuShaderDesc_setLanguage(JNIEnv * env, jobject self, jobject lang) {
    OCIO_JNITRY_ENTER()
    GpuShaderDescRcPtr ptr = GetEditableJOCIO<GpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    ptr->setLanguage(GetJEnum<GpuLanguage>(env, lang));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_GpuShaderDesc_getLanguage(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstGpuShaderDescRcPtr ptr = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/GpuLanguage", ptr->getLanguage());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GpuShaderDesc_setFunctionName(JNIEnv * env, jobject self, jstring name) {
    OCIO_JNITRY_ENTER()
    const char *_name = env->GetStringUTFChars(name, 0);
    GpuShaderDescRcPtr ptr = GetEditableJOCIO<GpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    ptr->setFunctionName(_name);
    env->ReleaseStringUTFChars(name, _name);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_GpuShaderDesc_getFunctionName(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstGpuShaderDescRcPtr ptr = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    return env->NewStringUTF(ptr->getFunctionName());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GpuShaderDesc_setLut3DEdgeLen(JNIEnv * env, jobject self, jint len) {
    OCIO_JNITRY_ENTER()
    GpuShaderDescRcPtr ptr = GetEditableJOCIO<GpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    ptr->setLut3DEdgeLen((int)len);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_GpuShaderDesc_getLut3DEdgeLen(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstGpuShaderDescRcPtr ptr = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    return (jint)ptr->getLut3DEdgeLen();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_GpuShaderDesc_getCacheID(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    ConstGpuShaderDescRcPtr ptr = GetConstJOCIO<ConstGpuShaderDescRcPtr, GpuShaderDescJNI>(env, self);
    return env->NewStringUTF(ptr->getCacheID());
    OCIO_JNITRY_EXIT(NULL)
}
