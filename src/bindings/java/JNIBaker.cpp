// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <string>
#include <sstream>

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<BakerJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Baker_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<BakerRcPtr, BakerJNI>(env, self,
      env->FindClass("org/OpenColorIO/Baker"), Baker::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Baker_createEditableCopy(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return BuildJObject<BakerRcPtr, BakerJNI>(env, self,
      env->FindClass("org/OpenColorIO/Baker"), bake->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setConfig(JNIEnv * env, jobject self, jobject config)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, config);
    bake->setConfig(cfg);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Baker_getConfig(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return BuildJConstObject<ConstConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"), bake->getConfig());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setFormat(JNIEnv * env, jobject self, jstring formatName)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setFormat(GetJStringValue(env, formatName)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getFormat(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getFormat());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setType(JNIEnv * env, jobject self, jstring type)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setType(GetJStringValue(env, type)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getType(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getType());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setMetadata(JNIEnv * env, jobject self, jstring metadata)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setMetadata(GetJStringValue(env, metadata)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getMetadata(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getMetadata());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setInputSpace(JNIEnv * env, jobject self, jstring inputSpace)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setInputSpace(GetJStringValue(env, inputSpace)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getInputSpace(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getInputSpace());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setShaperSpace(JNIEnv * env, jobject self, jstring shaperSpace)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setShaperSpace(GetJStringValue(env, shaperSpace)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getShaperSpace(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getShaperSpace());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setLooks(JNIEnv * env, jobject self, jstring looks)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setLooks(GetJStringValue(env, looks)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getLooks(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getLooks());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setTargetSpace(JNIEnv * env, jobject self, jstring targetSpace)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    bake->setTargetSpace(GetJStringValue(env, targetSpace)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getTargetSpace(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getTargetSpace());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setShaperSize(JNIEnv * env, jobject self, jint shapersize)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    return bake->setShaperSize((int)shapersize);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Baker_getShaperSize(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return (jint)bake->getShaperSize();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Baker_setCubeSize(JNIEnv * env, jobject self, jint cubesize)
{
    OCIO_JNITRY_ENTER()
    BakerRcPtr bake = GetEditableJOCIO<BakerRcPtr, BakerJNI>(env, self);
    return bake->setCubeSize((int)cubesize);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Baker_getCubeSize(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return (jint)bake->getCubeSize();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_bake(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    std::ostringstream os;
    bake->bake(os);
    return env->NewStringUTF(os.str().c_str());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Baker_getNumFormats(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return (jint)bake->getNumFormats();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getFormatNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getFormatNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Baker_getFormatExtensionByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstBakerRcPtr bake = GetConstJOCIO<ConstBakerRcPtr, BakerJNI>(env, self);
    return env->NewStringUTF(bake->getFormatExtensionByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}
