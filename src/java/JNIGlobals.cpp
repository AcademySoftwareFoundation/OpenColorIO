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

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Globals_create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    jfieldID fid;
    jclass wclass = env->GetObjectClass(self);
    fid = env->GetFieldID(wclass, "ROLE_DEFAULT", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_DEFAULT));
    fid = env->GetFieldID(wclass, "ROLE_REFERENCE", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_REFERENCE));
    fid = env->GetFieldID(wclass, "ROLE_DATA", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_DATA));
    fid = env->GetFieldID(wclass, "ROLE_COLOR_PICKING", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_COLOR_PICKING));
    fid = env->GetFieldID(wclass, "ROLE_SCENE_LINEAR", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_SCENE_LINEAR));
    fid = env->GetFieldID(wclass, "ROLE_COMPOSITING_LOG", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_COMPOSITING_LOG));
    fid = env->GetFieldID(wclass, "ROLE_COLOR_TIMING", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_COLOR_TIMING));
    fid = env->GetFieldID(wclass, "ROLE_TEXTURE_PAINT", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_TEXTURE_PAINT));
    fid = env->GetFieldID(wclass, "ROLE_MATTE_PAINT", "Ljava/lang/String;");
    env->SetObjectField(self, fid, env->NewStringUTF(ROLE_MATTE_PAINT));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Globals_ClearAllCaches(JNIEnv * env, jobject)
{
    OCIO_JNITRY_ENTER()
    ClearAllCaches();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_GetVersion(JNIEnv * env, jobject)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(GetVersion());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Globals_GetVersionHex(JNIEnv * env, jobject)
{
    OCIO_JNITRY_ENTER()
    return (jint)GetVersionHex();
    OCIO_JNITRY_EXIT(-1)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_GetLoggingLevel(JNIEnv * env, jobject)
{
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/LoggingLevel", GetLoggingLevel());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Globals_SetLoggingLevel(JNIEnv * env, jobject, jobject level)
{
    OCIO_JNITRY_ENTER()
    SetLoggingLevel(GetJEnum<LoggingLevel>(env, level));
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_GetCurrentConfig(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    jobject obj = BuildJConstObject<ConstConfigRcPtr, ConfigJNI>(env, self,
        env->FindClass("org/OpenColorIO/Config"), GetCurrentConfig());
    return obj;
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Globals_SetCurrentConfig(JNIEnv * env, jobject, jobject config)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, config);
    SetCurrentConfig(cfg);
    OCIO_JNITRY_EXIT()
}

// Bool

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_BoolToString(JNIEnv * env, jobject, jboolean val)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(BoolToString((bool)val));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Globals_BoolFromString(JNIEnv * env, jobject, jstring s)
{
    OCIO_JNITRY_ENTER()
    return (jboolean)BoolFromString(GetJStringValue(env, s)());
    OCIO_JNITRY_EXIT(false)
}

// LoggingLevel

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_LoggingLevel_toString(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      LoggingLevelToString(GetJEnum<LoggingLevel>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_LoggingLevel_equals(JNIEnv * env, jobject self, jobject obj)
{
    OCIO_JNITRY_ENTER()
    return GetJEnum<LoggingLevel>(env, self)
        == GetJEnum<LoggingLevel>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_LoggingLevelToString(JNIEnv * env, jobject, jobject level)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      LoggingLevelToString(GetJEnum<LoggingLevel>(env, level)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_LoggingLevelFromString(JNIEnv * env, jobject, jstring s)
{
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/LoggingLevel",
             LoggingLevelFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

// TransformDirection

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_TransformDirection_toString(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      TransformDirectionToString(GetJEnum<TransformDirection>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_TransformDirection_equals(JNIEnv * env, jobject self, jobject obj)
{
    OCIO_JNITRY_ENTER()
    return GetJEnum<TransformDirection>(env, self)
        == GetJEnum<TransformDirection>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_TransformDirectionToString(JNIEnv * env, jobject,
                                                      jobject dir)
{
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      TransformDirectionToString(GetJEnum<TransformDirection>(env, dir)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_TransformDirectionFromString(JNIEnv * env, jobject,
                                                        jstring s)
{
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/TransformDirection",
             TransformDirectionFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_GetInverseTransformDirection(JNIEnv * env, jobject,
                                                        jobject dir) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/TransformDirection", 
      GetInverseTransformDirection(GetJEnum<TransformDirection>(env, dir)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_CombineTransformDirections(JNIEnv * env, jobject,
                                                      jobject d1, jobject d2) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/TransformDirection",
      CombineTransformDirections(GetJEnum<TransformDirection>(env, d1),
                                 GetJEnum<TransformDirection>(env, d2)));
    OCIO_JNITRY_EXIT(NULL)
}

// ColorSpaceDirection

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_ColorSpaceDirection_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      ColorSpaceDirectionToString(GetJEnum<ColorSpaceDirection>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_ColorSpaceDirection_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<ColorSpaceDirection>(env, self)
        == GetJEnum<ColorSpaceDirection>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_ColorSpaceDirectionToString(JNIEnv * env, jobject, jobject dir) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      ColorSpaceDirectionToString(GetJEnum<ColorSpaceDirection>(env, dir)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_ColorSpaceDirectionFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/ColorSpaceDirection",
             ColorSpaceDirectionFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

// BitDepth

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_BitDepth_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      BitDepthToString(GetJEnum<BitDepth>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_BitDepth_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<BitDepth>(env, self)
        == GetJEnum<BitDepth>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_BitDepthToString(JNIEnv * env, jobject, jobject bitDepth) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      BitDepthToString(GetJEnum<BitDepth>(env, bitDepth)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_BitDepthFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/BitDepth",
             BitDepthFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Globals_BitDepthIsFloat(JNIEnv * env, jobject, jobject bitDepth) {
    OCIO_JNITRY_ENTER()
    return (jboolean)BitDepthIsFloat(GetJEnum<BitDepth>(env, bitDepth));
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Globals_BitDepthToInt(JNIEnv * env, jobject, jobject bitDepth) {
    OCIO_JNITRY_ENTER()
    return (jint) BitDepthToInt(GetJEnum<BitDepth>(env, bitDepth));
    OCIO_JNITRY_EXIT(-1)
}

// Allocation

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Allocation_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      AllocationToString(GetJEnum<Allocation>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Allocation_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<Allocation>(env, self)
        == GetJEnum<Allocation>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_AllocationToString(JNIEnv * env, jobject, jobject allocation) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      AllocationToString(GetJEnum<Allocation>(env, allocation)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_AllocationFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/Allocation",
             AllocationFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

// Interpolation

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Interpolation_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      InterpolationToString(GetJEnum<Interpolation>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Interpolation_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<Interpolation>(env, self)
        == GetJEnum<Interpolation>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_InterpolationToString(JNIEnv * env, jobject, jobject interp) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      InterpolationToString(GetJEnum<Interpolation>(env, interp)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_InterpolationFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/Interpolation",
             InterpolationFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

// GpuLanguage

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_GpuLanguage_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      GpuLanguageToString(GetJEnum<GpuLanguage>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_GpuLanguage_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<GpuLanguage>(env, self)
        == GetJEnum<GpuLanguage>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_GpuLanguageToString(JNIEnv * env, jobject, jobject language) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      GpuLanguageToString(GetJEnum<GpuLanguage>(env, language)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_GpuLanguageFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/GpuLanguage",
             GpuLanguageFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}

// EnvironmentMode

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_EnvironmentMode_toString(JNIEnv * env, jobject self) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      EnvironmentModeToString(GetJEnum<EnvironmentMode>(env, self)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_EnvironmentMode_equals(JNIEnv * env, jobject self, jobject obj) {
    OCIO_JNITRY_ENTER()
    return GetJEnum<EnvironmentMode>(env, self)
        == GetJEnum<EnvironmentMode>(env, obj);
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Globals_EnvironmentModeToString(JNIEnv * env, jobject, jobject mode) {
    OCIO_JNITRY_ENTER()
    return env->NewStringUTF(
      EnvironmentModeToString(GetJEnum<EnvironmentMode>(env, mode)));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Globals_EnvironmentModeFromString(JNIEnv * env, jobject, jstring s) {
    OCIO_JNITRY_ENTER()
    return BuildJEnum(env, "org/OpenColorIO/EnvironmentMode",
             EnvironmentModeFromString(GetJStringValue(env, s)()));
    OCIO_JNITRY_EXIT(NULL)
}
