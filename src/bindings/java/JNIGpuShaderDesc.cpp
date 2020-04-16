// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>
#include <sstream>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

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
