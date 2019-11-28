// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>
#include <sstream>
#include <vector>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<LookJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Look_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<LookRcPtr, LookJNI>(env, self,
             env->FindClass("org/OpenColorIO/Look"), Look::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Look_getName(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, self);
    return env->NewStringUTF(lok->getName());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_setName(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    LookRcPtr lok = GetEditableJOCIO<LookRcPtr, LookJNI>(env, self);
    lok->setName(GetJStringValue(env, name)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Look_getProcessSpace(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, self);
    return env->NewStringUTF(lok->getProcessSpace());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_setProcessSpace(JNIEnv * env, jobject self, jstring processSpace)
{
    OCIO_JNITRY_ENTER()
    LookRcPtr lok = GetEditableJOCIO<LookRcPtr, LookJNI>(env, self);
    lok->setProcessSpace(GetJStringValue(env, processSpace)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Look_getDescription(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, self);
    return env->NewStringUTF(lok->getDescription());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_setDescription(JNIEnv * env, jobject self, jstring processSpace)
{
    OCIO_JNITRY_ENTER()
    LookRcPtr lok = GetEditableJOCIO<LookRcPtr, LookJNI>(env, self);
    lok->setDescription(GetJStringValue(env, processSpace)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Look_getTransform(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, self);
    ConstTransformRcPtr tr = lok->getTransform();
    return BuildJConstObject<ConstTransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(tr)), tr);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_setTransform(JNIEnv * env, jobject self, jobject transform)
{
    OCIO_JNITRY_ENTER()
    LookRcPtr lok = GetEditableJOCIO<LookRcPtr, LookJNI>(env, self);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    lok->setTransform(tran);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Look_getInverseTransform(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, self);
    ConstTransformRcPtr tr = lok->getInverseTransform();
    return BuildJConstObject<ConstTransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(tr)), tr);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Look_setInverseTransform(JNIEnv * env, jobject self, jobject transform)
{
    OCIO_JNITRY_ENTER()
    LookRcPtr lok = GetEditableJOCIO<LookRcPtr, LookJNI>(env, self);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    lok->setInverseTransform(tran);
    OCIO_JNITRY_EXIT()
}
