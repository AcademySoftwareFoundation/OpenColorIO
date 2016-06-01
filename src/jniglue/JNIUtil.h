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

#ifndef INCLUDED_OCIO_JNIUTIL_H
#define INCLUDED_OCIO_JNIUTIL_H

#include <sstream>
#include <vector>
#include "OpenColorIOJNI.h"

OCIO_NAMESPACE_ENTER
{

template<typename C, typename E>
struct JObject
{
    jobject back_ptr;
    C * constcppobj;
    E * cppobj;
    bool isconst;
};

typedef OCIO_SHARED_PTR<const GpuShaderDesc> ConstGpuShaderDescRcPtr;
typedef OCIO_SHARED_PTR<GpuShaderDesc> GpuShaderDescRcPtr;
typedef OCIO_SHARED_PTR<const ImageDesc> ConstImageDescRcPtr;
typedef OCIO_SHARED_PTR<ImageDesc> ImageDescRcPtr;
typedef OCIO_SHARED_PTR<const PackedImageDesc> ConstPackedImageDescRcPtr;
typedef OCIO_SHARED_PTR<PackedImageDesc> PackedImageDescRcPtr;
typedef OCIO_SHARED_PTR<const PlanarImageDesc> ConstPlanarImageDescRcPtr;
typedef OCIO_SHARED_PTR<PlanarImageDesc> PlanarImageDescRcPtr;

typedef JObject <ConstConfigRcPtr, ConfigRcPtr> ConfigJNI;
typedef JObject <ConstContextRcPtr, ContextRcPtr> ContextJNI;
typedef JObject <ConstProcessorRcPtr, ProcessorRcPtr> ProcessorJNI;
typedef JObject <ConstColorSpaceRcPtr, ColorSpaceRcPtr> ColorSpaceJNI;
typedef JObject <ConstLookRcPtr, LookRcPtr> LookJNI;
typedef JObject <ConstBakerRcPtr, BakerRcPtr> BakerJNI;
typedef JObject <ConstGpuShaderDescRcPtr, GpuShaderDescRcPtr> GpuShaderDescJNI;
typedef JObject <ConstImageDescRcPtr, ImageDescRcPtr> ImageDescJNI;
typedef JObject <ConstTransformRcPtr, TransformRcPtr> TransformJNI;
typedef JObject <ConstAllocationTransformRcPtr, AllocationTransformRcPtr> AllocationTransformJNI;
typedef JObject <ConstCDLTransformRcPtr, CDLTransformRcPtr> CDLTransformJNI;
typedef JObject <ConstCLampTransformRcPtr, ClampTransformRcPtr> ClampTransformJNI;
typedef JObject <ConstColorSpaceTransformRcPtr, ColorSpaceTransformRcPtr> ColorSpaceTransformJNI;
typedef JObject <ConstDisplayTransformRcPtr, DisplayTransformRcPtr> DisplayTransformJNI;
typedef JObject <ConstExponentTransformRcPtr, ExponentTransformRcPtr> ExponentTransformJNI;
typedef JObject <ConstFileTransformRcPtr, FileTransformRcPtr> FileTransformJNI;
typedef JObject <ConstGroupTransformRcPtr, GroupTransformRcPtr> GroupTransformJNI;
typedef JObject <ConstLogTransformRcPtr, LogTransformRcPtr> LogTransformJNI;
typedef JObject <ConstLookTransformRcPtr, LookTransformRcPtr> LookTransformJNI;
typedef JObject <ConstMatrixTransformRcPtr, MatrixTransformRcPtr> MatrixTransformJNI;
typedef JObject <ConstTruelightTransformRcPtr, TruelightTransformRcPtr> TruelightTransformJNI;

template<typename T, typename S>
inline jobject BuildJConstObject(JNIEnv * env, jobject self, jclass cls, T ptr)
{
    S * jnistruct = new S ();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new T ();
    *jnistruct->constcppobj = ptr;
    jnistruct->isconst = true;
    jmethodID mid = env->GetMethodID(cls, "<init>", "(J)V");
    return env->NewObject(cls, mid, (jlong)jnistruct);
}

template<typename T, typename S>
inline jobject BuildJObject(JNIEnv * env, jobject self, jclass cls, T ptr)
{
    S * jnistruct = new S ();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->cppobj = new T ();
    *jnistruct->cppobj = ptr;
    jnistruct->isconst = false;
    jmethodID mid = env->GetMethodID(cls, "<init>", "(J)V");
    return env->NewObject(cls, mid, (jlong)jnistruct);
}

template<typename S>
inline void DisposeJOCIO(JNIEnv * env, jobject self)
{
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    jlong m_impl = env->GetLongField(self, fid);
    if(m_impl == 0) return; // nothing to do 
    S * jni = reinterpret_cast<S *> (m_impl);
    delete jni->constcppobj;
    delete jni->cppobj;
    env->DeleteGlobalRef(jni->back_ptr);
    delete jni;
    env->SetLongField(self, fid, 0);
    return;
}

template<typename T, typename S>
inline T GetConstJOCIO(JNIEnv * env, jobject self)
{
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    jlong m_impl = env->GetLongField(self, fid);
    if(m_impl == 0) {
        throw Exception("Java object doesn't point to a OCIO object");
    }
    S * jni = reinterpret_cast<S *> (m_impl);
    if(jni->isconst && jni->constcppobj) {
        return *jni->constcppobj;
    }
    if(!jni->isconst && jni->cppobj) {
        return *jni->cppobj;
    }
    throw Exception("Could not get a const OCIO object");
}

template<typename T, typename S>
inline T GetEditableJOCIO(JNIEnv * env, jobject self)
{
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    jlong m_impl = env->GetLongField(self, fid);
    if(m_impl == 0) {
        throw Exception("Java object doesn't point to a OCIO object");
    }
    S * jni = reinterpret_cast<S *> (m_impl);
    if(!jni->isconst && jni->cppobj) {
        return *jni->cppobj;
    }
    throw Exception("Could not get an editable OCIO object");
}

template<typename T>
inline T GetJEnum(JNIEnv * env, jobject j_enum)
{
    jclass cls = env->GetObjectClass(j_enum);
    jfieldID fid = env->GetFieldID(cls, "m_enum", "I");
    return (T)env->GetIntField(j_enum, fid);
}

template<typename T>
inline jobject BuildJEnum(JNIEnv * env, const char* ociotype, T val)
{
    jclass cls = env->FindClass(ociotype);
    jmethodID mid = env->GetMethodID(cls, "<init>", "(I)V");
    return env->NewObject(cls, mid, (int)val);
}

template<typename T>
inline void CheckArrayLength(JNIEnv * env, const char* name, T ptr, int32_t length) {
    if(ptr == NULL) return;
    if(env->GetArrayLength(ptr) < length) {
        std::ostringstream err;
        err << name << " needs to have " << length;
        err << " elements but found only " << env->GetArrayLength(ptr);
        throw Exception(err.str().c_str());
    }
}

class GetJFloatArrayValue
{
public:
    GetJFloatArrayValue(JNIEnv* env, jfloatArray val, const char* name, int32_t len) {
        CheckArrayLength(env, name, val, len);
        m_env = env;
        m_val = val;
        if(val != NULL) m_ptr = env->GetFloatArrayElements(val, JNI_FALSE);
    }
    ~GetJFloatArrayValue() {
        if(m_val != NULL) m_env->ReleaseFloatArrayElements(m_val, m_ptr, JNI_FALSE);
        m_val = NULL;
        m_ptr = NULL;
    }
    jfloat* operator() () {
        return m_ptr;
    }
private:
    JNIEnv* m_env;
    jfloat* m_ptr;
    jfloatArray m_val;
};

class SetJFloatArrayValue
{
public:
    SetJFloatArrayValue(JNIEnv* env, jfloatArray val, const char* name, int32_t len) {
        CheckArrayLength(env, name, val, len);
        m_env = env;
        m_val = val;
        if(val != NULL) m_tmp.resize(len);
    }
    ~SetJFloatArrayValue() {
        if(m_val != NULL) m_env->SetFloatArrayRegion(m_val, 0, m_tmp.size(), &m_tmp[0]);
        m_val = NULL;
        m_tmp.clear();
    }
    float* operator() () {
        return &m_tmp[0];
    }
private:
    JNIEnv* m_env;
    std::vector<float> m_tmp;
    jfloatArray m_val;
};

class GetJIntArrayValue
{
public:
    GetJIntArrayValue(JNIEnv* env, jintArray val, const char* name, int32_t len) {
        CheckArrayLength(env, name, val, len);
        m_env = env;
        m_val = val;
        if(val != NULL) m_ptr = env->GetIntArrayElements(val, JNI_FALSE);
    }
    ~GetJIntArrayValue() {
        if(m_val != NULL) m_env->ReleaseIntArrayElements(m_val, m_ptr, JNI_FALSE);
        m_val = NULL;
        m_ptr = NULL;
    }
    jint* operator() () {
        return m_ptr;
    }
private:
    JNIEnv* m_env;
    jint* m_ptr;
    jintArray m_val;
};

class GetJStringValue
{
public:
    GetJStringValue(JNIEnv* env, jstring val) {
        m_env = env;
        m_val = val;
        if(val != NULL) m_tmp = env->GetStringUTFChars(m_val, 0);
    }
    ~GetJStringValue() {
        if(m_val != NULL) m_env->ReleaseStringUTFChars(m_val, m_tmp);
        m_val = NULL;
        m_tmp = NULL;
    }
    const char* operator() () {
        return m_tmp;
    }
private:
    JNIEnv* m_env;
    const char* m_tmp;
    jstring m_val;
};

jobject NewJFloatBuffer(JNIEnv * env, float* ptr, int32_t len);
float* GetJFloatBuffer(JNIEnv * env, jobject buffer, int32_t len);
const char* GetOCIOTClass(ConstTransformRcPtr tran);
void JNI_Handle_Exception(JNIEnv * env);

#define OCIO_JNITRY_ENTER() try {
#define OCIO_JNITRY_EXIT(ret) } catch(...) { JNI_Handle_Exception(env); return ret; }

}
OCIO_NAMESPACE_EXIT

#endif // INCLUDED_OCIO_JNIUTIL_H
