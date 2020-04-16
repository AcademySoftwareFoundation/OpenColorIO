// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>
#include <sstream>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<ContextJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Context_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<ContextRcPtr, ContextJNI>(env, self,
             env->FindClass("org/OpenColorIO/Context"), Context::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Context_createEditableCopy(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return BuildJObject<ContextRcPtr, ContextJNI>(env, self,
             env->FindClass("org/OpenColorIO/Context"), con->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_getCacheID(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->getCacheID());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_setSearchPath(JNIEnv * env, jobject self, jstring path)
{
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->setSearchPath(GetJStringValue(env, path)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_getSearchPath(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->getSearchPath());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_setWorkingDir(JNIEnv * env, jobject self, jstring dirname)
{
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->setWorkingDir(GetJStringValue(env, dirname)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_getWorkingDir(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->getWorkingDir());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_setStringVar(JNIEnv * env, jobject self, jstring name, jstring var)
{
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->setStringVar(GetJStringValue(env, name)(), GetJStringValue(env, var)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_getStringVar(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->getStringVar(GetJStringValue(env, name)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Context_getNumStringVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return (jint)con->getNumStringVars();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_getStringVarNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->getStringVarNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_clearStringVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->clearStringVars();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_setEnvironmentMode(JNIEnv * env, jobject self, jobject mode) {
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->setEnvironmentMode(GetJEnum<EnvironmentMode>(env, mode));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Context_getEnvironmentMode(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/EnvironmentMode", con->getEnvironmentMode());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Context_loadEnvironment(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ContextRcPtr con = GetEditableJOCIO<ContextRcPtr, ContextJNI>(env, self);
    con->loadEnvironment();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_resolveStringVar(JNIEnv * env, jobject self, jstring val)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->resolveStringVar(GetJStringValue(env, val)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Context_resolveFileLocation(JNIEnv * env, jobject self, jstring filename)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, self);
    return env->NewStringUTF(con->resolveFileLocation(GetJStringValue(env, filename)()));
    OCIO_JNITRY_EXIT(NULL)
}
