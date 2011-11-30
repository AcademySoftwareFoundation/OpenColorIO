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

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
OCIO_NAMESPACE_USING

// Transform

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Transform_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<TransformJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Transform_createEditableCopy(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTransformRcPtr ctran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, self);
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(ctran)), ctran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Transform_getDirection(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/TransformDirection", ptr->getDirection());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Transform_setDirection(JNIEnv * env, jobject self, jobject dir)
{
    OCIO_JNITRY_ENTER()
    TransformRcPtr ptr = GetEditableJOCIO<TransformRcPtr, TransformJNI>(env, self);
    ptr->setDirection(GetJEnum<TransformDirection>(env, dir));
    OCIO_JNITRY_EXIT()
}

// AllocationTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_AllocationTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<AllocationTransformRcPtr, AllocationTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/AllocationTransform"), AllocationTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_AllocationTransform_getAllocation(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstAllocationTransformRcPtr alctran = GetConstJOCIO<ConstAllocationTransformRcPtr, AllocationTransformJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/Allocation", alctran->getAllocation());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_AllocationTransform_setAllocation(JNIEnv * env, jobject self, jobject allocation)
{
    OCIO_JNITRY_ENTER()
    AllocationTransformRcPtr alctran = GetEditableJOCIO<AllocationTransformRcPtr, AllocationTransformJNI>(env, self);
    alctran->setAllocation(GetJEnum<Allocation>(env, allocation));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_AllocationTransform_getNumVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstAllocationTransformRcPtr alctran = GetConstJOCIO<ConstAllocationTransformRcPtr, AllocationTransformJNI>(env, self);
    return (jint)alctran->getNumVars();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_AllocationTransform_getVars(JNIEnv * env, jobject self, jfloatArray vars)
{
    OCIO_JNITRY_ENTER()
    ConstAllocationTransformRcPtr alctran = GetConstJOCIO<ConstAllocationTransformRcPtr, AllocationTransformJNI>(env, self);
    alctran->getVars(SetJFloatArrayValue(env, vars, "vars", alctran->getNumVars())());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_AllocationTransform_setVars(JNIEnv * env, jobject self, jint numvars, jfloatArray vars)
{
    OCIO_JNITRY_ENTER()
    AllocationTransformRcPtr alctran = GetEditableJOCIO<AllocationTransformRcPtr, AllocationTransformJNI>(env, self);
    alctran->setVars((int)numvars, GetJFloatArrayValue(env, vars, "vars", numvars)());
    OCIO_JNITRY_EXIT()
}

// CDLTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_CDLTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<CDLTransformRcPtr, CDLTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/CDLTransform"), CDLTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_CDLTransform_CreateFromFile(JNIEnv * env, jobject self, jstring src, jstring cccid)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<CDLTransformRcPtr, CDLTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/CDLTransform"),
             CDLTransform::CreateFromFile(GetJStringValue(env, src)(), GetJStringValue(env, cccid)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_CDLTransform_equals(JNIEnv * env, jobject self, jobject obj)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr left = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    ConstCDLTransformRcPtr right = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, obj);
    return (jboolean)left->equals(right);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_CDLTransform_getXML(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    return env->NewStringUTF(cdltran->getXML());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setXML(JNIEnv * env, jobject self, jstring xml)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setXML(GetJStringValue(env, xml)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setSlope(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setSlope(GetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_getSlope(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->getSlope(SetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setOffset(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setOffset(GetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_getOffset(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->getOffset(SetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setPower(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setPower(GetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_getPower(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->getPower(SetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setSOP(JNIEnv * env, jobject self, jfloatArray vec9)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setSOP(GetJFloatArrayValue(env, vec9, "vec9", 9)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_getSOP(JNIEnv * env, jobject self, jfloatArray vec9)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->getSOP(SetJFloatArrayValue(env, vec9, "vec9", 9)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setSat(JNIEnv * env, jobject self, jfloat sat)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setSat((float)sat);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jfloat JNICALL
Java_org_OpenColorIO_CDLTransform_getSat(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    return (jfloat)cdltran->getSat();
    OCIO_JNITRY_EXIT(1.f)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_getSatLumaCoefs(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->getSatLumaCoefs(SetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setID(JNIEnv * env, jobject self, jstring id)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setID(GetJStringValue(env, id)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_CDLTransform_getID(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    return env->NewStringUTF(cdltran->getID());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_CDLTransform_setDescription(JNIEnv * env, jobject self, jstring desc)
{
    OCIO_JNITRY_ENTER()
    CDLTransformRcPtr cdltran = GetEditableJOCIO<CDLTransformRcPtr, CDLTransformJNI>(env, self);
    cdltran->setDescription(GetJStringValue(env, desc)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_CDLTransform_getDescription(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstCDLTransformRcPtr cdltran = GetConstJOCIO<ConstCDLTransformRcPtr, CDLTransformJNI>(env, self);
    return env->NewStringUTF(cdltran->getDescription());
    OCIO_JNITRY_EXIT(NULL)
}

// ColorSpaceTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ColorSpaceTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<ColorSpaceTransformRcPtr, ColorSpaceTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/ColorSpaceTransform"), ColorSpaceTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpaceTransform_getSrc(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceTransformRcPtr coltran = GetConstJOCIO<ConstColorSpaceTransformRcPtr, ColorSpaceTransformJNI>(env, self);
    return env->NewStringUTF(coltran->getSrc());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpaceTransform_setSrc(JNIEnv * env, jobject self, jstring src)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceTransformRcPtr coltran = GetEditableJOCIO<ColorSpaceTransformRcPtr, ColorSpaceTransformJNI>(env, self);
    coltran->setSrc(GetJStringValue(env, src)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpaceTransform_getDst(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstColorSpaceTransformRcPtr coltran = GetConstJOCIO<ConstColorSpaceTransformRcPtr, ColorSpaceTransformJNI>(env, self);
    return env->NewStringUTF(coltran->getDst());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ColorSpaceTransform_setDst(JNIEnv * env, jobject self, jstring dst)
{
    OCIO_JNITRY_ENTER()
    ColorSpaceTransformRcPtr coltran = GetEditableJOCIO<ColorSpaceTransformRcPtr, ColorSpaceTransformJNI>(env, self);
    coltran->setDst(GetJStringValue(env, dst)());
    OCIO_JNITRY_EXIT()
}

// DisplayTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_DisplayTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<DisplayTransformRcPtr, DisplayTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/DisplayTransform"), DisplayTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setInputColorSpaceName(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    dtran->setInputColorSpaceName(GetJStringValue(env, name)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_DisplayTransform_getInputColorSpaceName(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    return env->NewStringUTF(dtran->getInputColorSpaceName());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setLinearCC(JNIEnv * env, jobject self, jobject cc)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, cc);
    dtran->setLinearCC(ptr);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_DisplayTransform_getLinearCC(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr cctran = dtran->getLinearCC();
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(cctran)), cctran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setColorTimingCC(JNIEnv * env, jobject self, jobject cc)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, cc);
    dtran->setColorTimingCC(ptr);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_DisplayTransform_getColorTimingCC(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr cctran = dtran->getColorTimingCC();
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(cctran)), cctran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setChannelView(JNIEnv * env, jobject self, jobject transform)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    dtran->setChannelView(ptr);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_DisplayTransform_getChannelView(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr cvtran = dtran->getChannelView();
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(cvtran)), cvtran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setDisplay(JNIEnv * env, jobject self, jstring display)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    dtran->setDisplay(GetJStringValue(env, display)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_DisplayTransform_getDisplay(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    return env->NewStringUTF(dtran->getDisplay());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setView(JNIEnv * env, jobject self, jstring view)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    dtran->setView(GetJStringValue(env, view)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_DisplayTransform_getView(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    return env->NewStringUTF(dtran->getView());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setDisplayCC(JNIEnv * env, jobject self, jobject cc)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, cc);
    dtran->setDisplayCC(ptr);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_DisplayTransform_getDisplayCC(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    ConstTransformRcPtr cctran = dtran->getDisplayCC();
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(cctran)), cctran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setLooksOverride(JNIEnv * env, jobject self, jstring looks)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    dtran->setLooksOverride(GetJStringValue(env, looks)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_DisplayTransform_getLooksOverride(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    return env->NewStringUTF(dtran->getLooksOverride());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_DisplayTransform_setLooksOverrideEnabled(JNIEnv * env, jobject self, jboolean enabled)
{
    OCIO_JNITRY_ENTER()
    DisplayTransformRcPtr dtran = GetEditableJOCIO<DisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    dtran->setLooksOverrideEnabled((bool)enabled);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_DisplayTransform_getLooksOverrideEnabled(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstDisplayTransformRcPtr dtran = GetConstJOCIO<ConstDisplayTransformRcPtr, DisplayTransformJNI>(env, self);
    return (jboolean)dtran->getLooksOverrideEnabled();
    OCIO_JNITRY_EXIT(false)
}

// ExponentTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_ExponentTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/ExponentTransform"), ExponentTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ExponentTransform_setValue(JNIEnv * env, jobject self, jfloatArray vec4)
{
    OCIO_JNITRY_ENTER()
    ExponentTransformRcPtr exptran = GetEditableJOCIO<ExponentTransformRcPtr, ExponentTransformJNI>(env, self);
    exptran->setValue(GetJFloatArrayValue(env, vec4, "vec4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_ExponentTransform_getValue(JNIEnv * env, jobject self, jfloatArray vec4)
{
    OCIO_JNITRY_ENTER()
    ConstExponentTransformRcPtr exptran = GetConstJOCIO<ConstExponentTransformRcPtr, ExponentTransformJNI>(env, self);
    exptran->getValue(SetJFloatArrayValue(env, vec4, "vec4", 4)());
    OCIO_JNITRY_EXIT()
}

// FileTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_FileTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<FileTransformRcPtr, FileTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/FileTransform"), FileTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_FileTransform_getSrc(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return env->NewStringUTF(filetran->getSrc());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_FileTransform_setSrc(JNIEnv * env, jobject self, jstring src)
{
    OCIO_JNITRY_ENTER()
    FileTransformRcPtr filetran = GetEditableJOCIO<FileTransformRcPtr, FileTransformJNI>(env, self);
    filetran->setSrc(GetJStringValue(env, src)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_FileTransform_getCCCId(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return env->NewStringUTF(filetran->getCCCId());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_FileTransform_setCCCId(JNIEnv * env, jobject self, jstring id)
{
    OCIO_JNITRY_ENTER()
    FileTransformRcPtr filetran = GetEditableJOCIO<FileTransformRcPtr, FileTransformJNI>(env, self);
    filetran->setCCCId(GetJStringValue(env, id)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_FileTransform_getInterpolation(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return BuildJEnum(env, "org/OpenColorIO/Interpolation", filetran->getInterpolation());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_FileTransform_setInterpolation(JNIEnv * env, jobject self, jobject interp)
{
    OCIO_JNITRY_ENTER()
    FileTransformRcPtr filetran = GetEditableJOCIO<FileTransformRcPtr, FileTransformJNI>(env, self);
    filetran->setInterpolation(GetJEnum<Interpolation>(env, interp));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_FileTransform_getNumFormats(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return (jint)filetran->getNumFormats();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_FileTransform_getFormatNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return env->NewStringUTF(filetran->getFormatNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_FileTransform_getFormatExtensionByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstFileTransformRcPtr filetran = GetConstJOCIO<ConstFileTransformRcPtr, FileTransformJNI>(env, self);
    return env->NewStringUTF(filetran->getFormatExtensionByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

// GroupTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_GroupTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<GroupTransformRcPtr, GroupTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/GroupTransform"), GroupTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_GroupTransform_getTransform(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstGroupTransformRcPtr gtran = GetConstJOCIO<ConstGroupTransformRcPtr, GroupTransformJNI>(env, self);
    ConstTransformRcPtr ctran = gtran->getTransform((int)index);
    return BuildJObject<TransformRcPtr, TransformJNI>(env, self,
             env->FindClass(GetOCIOTClass(ctran)), ctran->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_GroupTransform_size(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstGroupTransformRcPtr gtran = GetConstJOCIO<ConstGroupTransformRcPtr, GroupTransformJNI>(env, self);
    return (jint)gtran->size();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GroupTransform_push_1back(JNIEnv * env, jobject self, jobject transform)
{
    OCIO_JNITRY_ENTER()
    GroupTransformRcPtr gtran = GetEditableJOCIO<GroupTransformRcPtr, GroupTransformJNI>(env, self);
    ConstTransformRcPtr ptr = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    gtran->push_back(ptr);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_GroupTransform_clear(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    GroupTransformRcPtr gtran = GetEditableJOCIO<GroupTransformRcPtr, GroupTransformJNI>(env, self);
    gtran->clear();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_GroupTransform_empty(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstGroupTransformRcPtr gtran = GetConstJOCIO<ConstGroupTransformRcPtr, GroupTransformJNI>(env, self);
    return (jboolean)gtran->empty();
    OCIO_JNITRY_EXIT(false)
}

// LogTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_LogTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<LogTransformRcPtr, LogTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/LogTransform"), LogTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_LogTransform_setBase(JNIEnv * env, jobject self, jfloat val)
{
    OCIO_JNITRY_ENTER()
    LogTransformRcPtr ltran = GetEditableJOCIO<LogTransformRcPtr, LogTransformJNI>(env, self);
    ltran->setBase((float)val);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jfloat JNICALL
Java_org_OpenColorIO_LogTransform_getBase(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLogTransformRcPtr ltran = GetConstJOCIO<ConstLogTransformRcPtr, LogTransformJNI>(env, self);
    return (jfloat)ltran->getBase();
    OCIO_JNITRY_EXIT(0.f)
}

// LookTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_LookTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<LookTransformRcPtr, LookTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/LookTransform"), LookTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_LookTransform_getSrc(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookTransformRcPtr lktran = GetConstJOCIO<ConstLookTransformRcPtr, LookTransformJNI>(env, self);
    return env->NewStringUTF(lktran->getSrc());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_LookTransform_setSrc(JNIEnv * env, jobject self, jstring src)
{
    OCIO_JNITRY_ENTER()
    LookTransformRcPtr lktran = GetEditableJOCIO<LookTransformRcPtr, LookTransformJNI>(env, self);
    lktran->setSrc(GetJStringValue(env, src)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_LookTransform_getDst(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookTransformRcPtr lktran = GetConstJOCIO<ConstLookTransformRcPtr, LookTransformJNI>(env, self);
    return env->NewStringUTF(lktran->getDst());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_LookTransform_setDst(JNIEnv * env, jobject self, jstring dst)
{
    OCIO_JNITRY_ENTER()
    LookTransformRcPtr lktran = GetEditableJOCIO<LookTransformRcPtr, LookTransformJNI>(env, self);
    lktran->setDst(GetJStringValue(env, dst)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_LookTransform_setLooks(JNIEnv * env, jobject self, jstring looks)
{
    OCIO_JNITRY_ENTER()
    LookTransformRcPtr lktran = GetEditableJOCIO<LookTransformRcPtr, LookTransformJNI>(env, self);
    lktran->setLooks(GetJStringValue(env, looks)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_LookTransform_getLooks(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstLookTransformRcPtr lktran = GetConstJOCIO<ConstLookTransformRcPtr, LookTransformJNI>(env, self);
    return env->NewStringUTF(lktran->getLooks());
    OCIO_JNITRY_EXIT(NULL)
}

// MatrixTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_MatrixTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<MatrixTransformRcPtr, MatrixTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/MatrixTransform"), MatrixTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_MatrixTransform_equals(JNIEnv * env, jobject self, jobject obj)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr left = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    ConstMatrixTransformRcPtr right = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, obj);
    return (jboolean)left->equals(*right.get());
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_setValue(JNIEnv * env, jobject self, jfloatArray m44, jfloatArray offset4)
{
    OCIO_JNITRY_ENTER()
    MatrixTransformRcPtr mtran = GetEditableJOCIO<MatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->setValue(GetJFloatArrayValue(env, m44, "m44", 16)(),
                    GetJFloatArrayValue(env, offset4, "offset4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_getValue(JNIEnv * env, jobject self, jfloatArray m44, jfloatArray offset4)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->getValue(SetJFloatArrayValue(env, m44, "m44", 16)(),
                    SetJFloatArrayValue(env, offset4, "offset4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_setMatrix(JNIEnv * env, jobject self, jfloatArray m44)
{
    OCIO_JNITRY_ENTER()
    MatrixTransformRcPtr mtran = GetEditableJOCIO<MatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->setMatrix(GetJFloatArrayValue(env, m44, "m44", 16)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_getMatrix(JNIEnv * env, jobject self, jfloatArray m44)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->getMatrix(SetJFloatArrayValue(env, m44, "m44", 16)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_setOffset(JNIEnv * env, jobject self, jfloatArray offset4)
{
    OCIO_JNITRY_ENTER()
    MatrixTransformRcPtr mtran = GetEditableJOCIO<MatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->setOffset(GetJFloatArrayValue(env, offset4, "offset4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_getOffset(JNIEnv * env, jobject self, jfloatArray offset4)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->getOffset(SetJFloatArrayValue(env, offset4, "offset4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_Fit(JNIEnv * env, jobject self,
    jfloatArray m44, jfloatArray offset4, jfloatArray oldmin4,
    jfloatArray oldmax4, jfloatArray newmin4, jfloatArray newmax4)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->Fit(GetJFloatArrayValue(env, m44, "m44", 16)(),
               GetJFloatArrayValue(env, offset4, "offset4", 4)(),
               GetJFloatArrayValue(env, oldmin4, "oldmin4", 4)(),
               GetJFloatArrayValue(env, oldmax4, "oldmax4", 4)(),
               GetJFloatArrayValue(env, newmin4, "newmin4", 4)(),
               GetJFloatArrayValue(env, newmax4, "newmax4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_Identity(JNIEnv * env, jobject self,
    jfloatArray m44, jfloatArray offset4)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->Identity(GetJFloatArrayValue(env, m44, "m44", 16)(),
                    GetJFloatArrayValue(env, offset4, "offset4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_Sat(JNIEnv * env, jobject self,
    jfloatArray m44, jfloatArray offset4, jfloat sat,
    jfloatArray lumaCoef3)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->Sat(GetJFloatArrayValue(env, m44, "m44", 16)(),
               GetJFloatArrayValue(env, offset4, "offset4", 4)(),
               (float)sat,
               GetJFloatArrayValue(env, lumaCoef3, "lumaCoef3", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_Scale(JNIEnv * env, jobject self,
    jfloatArray m44, jfloatArray offset4, jfloatArray scale4)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->Scale(GetJFloatArrayValue(env, m44, "m44", 16)(),
                 GetJFloatArrayValue(env, offset4, "offset4", 4)(),
                 GetJFloatArrayValue(env, scale4, "scale4", 4)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_MatrixTransform_View(JNIEnv * env, jobject self,
    jfloatArray m44, jfloatArray offset4, jintArray channelHot4,
    jfloatArray lumaCoef3)
{
    OCIO_JNITRY_ENTER()
    ConstMatrixTransformRcPtr mtran = GetConstJOCIO<ConstMatrixTransformRcPtr, MatrixTransformJNI>(env, self);
    mtran->View(GetJFloatArrayValue(env, m44, "m44", 16)(),
                GetJFloatArrayValue(env, offset4, "offset4", 4)(),
                GetJIntArrayValue(env, channelHot4, "channelHot4", 4)(),
                GetJFloatArrayValue(env, lumaCoef3, "lumaCoef3", 3)());
    OCIO_JNITRY_EXIT()
}

// TruelightTransform

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_TruelightTransform_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<TruelightTransformRcPtr, TruelightTransformJNI>(env, self,
             env->FindClass("org/OpenColorIO/TruelightTransform"), TruelightTransform::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setConfigRoot(JNIEnv * env, jobject self, jstring configroot)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setConfigRoot(GetJStringValue(env, configroot)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getConfigRoot(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getConfigRoot());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setProfile(JNIEnv * env, jobject self, jstring profile)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setProfile(GetJStringValue(env, profile)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getProfile(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getProfile());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setCamera(JNIEnv * env, jobject self, jstring camera)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setCamera(GetJStringValue(env, camera)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getCamera(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getCamera());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setInputDisplay(JNIEnv * env, jobject self, jstring display)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setInputDisplay(GetJStringValue(env, display)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getInputDisplay(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getInputDisplay());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setRecorder(JNIEnv * env, jobject self, jstring recorder)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setRecorder(GetJStringValue(env, recorder)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getRecorder(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getRecorder());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setPrint(JNIEnv * env, jobject self, jstring print)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setPrint(GetJStringValue(env, print)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getPrint(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getPrint());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setLamp(JNIEnv * env, jobject self, jstring lamp)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setLamp(GetJStringValue(env, lamp)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getLamp(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getLamp());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setOutputCamera(JNIEnv * env, jobject self, jstring camera)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setOutputCamera(GetJStringValue(env, camera)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getOutputCamera(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getOutputCamera());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setDisplay(JNIEnv * env, jobject self, jstring display)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setDisplay(GetJStringValue(env, display)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getDisplay(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getDisplay());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_TruelightTransform_setCubeInput(JNIEnv * env, jobject self, jstring type)
{
    OCIO_JNITRY_ENTER()
    TruelightTransformRcPtr ttran = GetEditableJOCIO<TruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    ttran->setCubeInput(GetJStringValue(env, type)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TruelightTransform_getCubeInput(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstTruelightTransformRcPtr ttran = GetConstJOCIO<ConstTruelightTransformRcPtr, TruelightTransformJNI>(env, self);
    return env->NewStringUTF(ttran->getCubeInput());
    OCIO_JNITRY_EXIT(NULL)
}
