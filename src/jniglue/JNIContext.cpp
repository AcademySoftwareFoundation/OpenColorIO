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
