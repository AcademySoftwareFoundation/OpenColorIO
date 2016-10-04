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
