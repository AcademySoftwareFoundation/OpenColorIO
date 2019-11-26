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
Java_org_OpenColorIO_ColorSpace_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<ColorSpaceJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpace_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<ColorSpaceRcPtr, ColorSpaceJNI>(env, self,
             env->FindClass("org/OpenColorIO/ColorSpace"), ColorSpace::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpace_createEditableCopy(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return BuildJObject<ColorSpaceRcPtr, ColorSpaceJNI>(env, self,
             env->FindClass("org/OpenColorIO/ColorSpace"), col->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpace_getName(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return env->NewStringUTF(col->getName());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setName(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setName(GetJStringValue(env, name)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpace_getFamily(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return env->NewStringUTF(col->getFamily());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setFamily(JNIEnv * env, jobject self, jstring family)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setFamily(GetJStringValue(env, family)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpace_getEqualityGroup(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return env->NewStringUTF(col->getEqualityGroup());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setEqualityGroup(JNIEnv * env, jobject self, jstring equalityGroup)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setEqualityGroup(GetJStringValue(env, equalityGroup)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpace_getDescription(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return env->NewStringUTF(col->getDescription());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setDescription(JNIEnv * env, jobject self, jstring description)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setDescription(GetJStringValue(env, description)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpace_getBitDepth(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/BitDepth", col->getBitDepth());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setBitDepth(JNIEnv * env, jobject self, jobject bitDepth)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setBitDepth(GetJEnum<BitDepth>(env, bitDepth));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_ColorSpace_isData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return (jboolean)col->isData();
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setIsData(JNIEnv * env, jobject self, jboolean isData)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setIsData((bool)isData);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpace_getAllocation(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/Allocation", col->getAllocation());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setAllocation(JNIEnv * env, jobject self, jobject allocation)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setAllocation(GetJEnum<Allocation>(env, allocation));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_ColorSpace_getAllocationNumVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    return (jint)col->getAllocationNumVars();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_getAllocationVars(JNIEnv * env, jobject self, jfloatArray vars)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->getAllocationVars(SetJFloatArrayValue(env, vars, "vars", col->getAllocationNumVars())());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setAllocationVars(JNIEnv * env, jobject self, jint numvars, jfloatArray vars)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    col->setAllocationVars((int)numvars, GetJFloatArrayValue(env, vars, "vars", numvars)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpace_getTransform(JNIEnv * env, jobject self, jobject dir)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceRcPtr col = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    ColorSpaceDirection cd = GetJEnum<ColorSpaceDirection>(env, dir);
    ConstTransformRcPtr tr = col->getTransform(cd);
    return BuildJConstObject<ConstTransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(tr)), tr);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpace_setTransform(JNIEnv * env, jobject self, jobject transform, jobject dir)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceRcPtr col = GetEditableJOCIO<ColorSpaceRcPtr, ColorSpaceJNI>(env, self);
    ColorSpaceDirection cd = GetJEnum<ColorSpaceDirection>(env, dir);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    col->setTransform(tran, cd);
    OCIO_JNITRY_EXIT()
}
