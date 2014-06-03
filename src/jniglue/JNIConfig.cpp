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
Java_org_OpenColorIO_Config_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    DisposeJOCIO<ConfigJNI>(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_Create(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJObject<ConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"), Config::Create());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_CreateFromEnv(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    return BuildJConstObject<ConstConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"), Config::CreateFromEnv());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_CreateFromFile(JNIEnv * env, jobject self, jstring filename)
{
    OCIO_JNITRY_ENTER()
    return BuildJConstObject<ConstConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"),
             Config::CreateFromFile(GetJStringValue(env, filename)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_CreateFromStream(JNIEnv * env, jobject self, jstring istream)
{
    OCIO_JNITRY_ENTER()
    std::istringstream is;
    is.str(std::string(GetJStringValue(env, istream)()));
    return BuildJConstObject<ConstConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"), Config::CreateFromStream(is));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_createEditableCopy(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJObject<ConfigRcPtr, ConfigJNI>(env, self,
             env->FindClass("org/OpenColorIO/Config"), cfg->createEditableCopy());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_sanityCheck(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    cfg->sanityCheck();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDescription(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDescription());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setDescription(JNIEnv * env, jobject self, jstring description)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setDescription(GetJStringValue(env, description)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_serialize(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    std::ostringstream os;
    cfg->serialize(os);
    return env->NewStringUTF(os.str().c_str());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getCacheID__(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getCacheID());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getCacheID__Lorg_OpenColorIO_Context_2(JNIEnv * env, jobject self, jobject context)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, context);
    return env->NewStringUTF(cfg->getCacheID(con));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getCurrentContext(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJConstObject<ConstContextRcPtr, ContextJNI>(env, self,
             env->FindClass("org/OpenColorIO/Context"), cfg->getCurrentContext());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_addEnvironmentVar(JNIEnv * env, jobject self, jstring name, jstring defaultValue)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->addEnvironmentVar(GetJStringValue(env, name)(), GetJStringValue(env, defaultValue)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumEnvironmentVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumEnvironmentVars();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getEnvironmentVarNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getEnvironmentVarNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getEnvironmentVarDefault(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getEnvironmentVarDefault(GetJStringValue(env, name)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_clearEnvironmentVars(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->clearEnvironmentVars();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getSearchPath(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getSearchPath());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setSearchPath(JNIEnv * env, jobject self, jstring path)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setSearchPath(GetJStringValue(env, path)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getWorkingDir(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getWorkingDir());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setWorkingDir(JNIEnv * env, jobject self, jstring dirname)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setWorkingDir(GetJStringValue(env, dirname)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumColorSpaces(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumColorSpaces();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getColorSpaceNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getColorSpaceNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getColorSpace(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJConstObject<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, self,
             env->FindClass("org/OpenColorIO/ColorSpace"),
             cfg->getColorSpace(GetJStringValue(env, name)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getIndexForColorSpace(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getIndexForColorSpace(GetJStringValue(env, name)());
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_addColorSpace(JNIEnv * env, jobject self, jobject cs)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    ConstColorSpaceRcPtr space = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, cs);
    cfg->addColorSpace(space);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_clearColorSpaces(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->clearColorSpaces();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_parseColorSpaceFromString(JNIEnv * env, jobject self, jstring str)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->parseColorSpaceFromString(GetJStringValue(env, str)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Config_isStrictParsingEnabled(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jboolean)cfg->isStrictParsingEnabled();
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setStrictParsingEnabled(JNIEnv * env, jobject self, jboolean enabled)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setStrictParsingEnabled((bool)enabled);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setRole(JNIEnv * env, jobject self, jstring role, jstring colorSpaceName)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setRole(GetJStringValue(env, role)(), GetJStringValue(env, colorSpaceName)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumRoles(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumRoles();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jboolean JNICALL
Java_org_OpenColorIO_Config_hasRole(JNIEnv * env, jobject self, jstring role)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jboolean)cfg->hasRole(GetJStringValue(env, role)());
    OCIO_JNITRY_EXIT(false)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getRoleName(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getRoleName((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDefaultDisplay(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDefaultDisplay());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumDisplays(JNIEnv * env, jobject self)
{
    return Java_org_OpenColorIO_Config_getNumDisplaysActive(env, self);
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDisplay(JNIEnv * env, jobject self, jint index)
{
    return Java_org_OpenColorIO_Config_getDisplayActive(env, self, index);
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumDisplaysActive(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumDisplaysActive();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDisplayActive(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDisplayActive((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumDisplaysAll(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumDisplaysAll();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDisplayAll(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDisplayAll((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDefaultView(JNIEnv * env, jobject self, jstring display)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDefaultView(GetJStringValue(env, display)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumViews(JNIEnv * env, jobject self, jstring display)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumViews(GetJStringValue(env, display)());
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getView(JNIEnv * env, jobject self, jstring display, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getView(GetJStringValue(env, display)(), (int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDisplayColorSpaceName(JNIEnv * env, jobject self, jstring display, jstring view)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDisplayColorSpaceName(GetJStringValue(env, display)(),
                                                           GetJStringValue(env, view)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getDisplayLooks(JNIEnv * env, jobject self, jstring display, jstring view)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getDisplayLooks(GetJStringValue(env, display)(),
                                                  GetJStringValue(env, view)()));
    OCIO_JNITRY_EXIT(NULL)
}

// TODO: seems that 4 string params causes a memory error in the JNI layer?
/*
JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_addDisplay(JNIEnv * env, jobject self, jstring display, jstring view, jstring colorSpaceName, jstring looks)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->addDisplay(GetJStringValue(env, display)(),
                    GetJStringValue(env, view)(),
                    GetJStringValue(env, colorSpaceName)(),
                    GetJStringValue(env, looks)());
    
    OCIO_JNITRY_EXIT()
}
*/

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_clearDisplays(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->clearDisplays();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setActiveDisplays(JNIEnv * env, jobject self, jstring displays)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setActiveDisplays(GetJStringValue(env, displays)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getActiveDisplays(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getActiveDisplays());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setActiveViews(JNIEnv * env, jobject self, jstring views)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setActiveViews(GetJStringValue(env, views)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getActiveViews(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getActiveViews());
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_getDefaultLumaCoefs(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    cfg->getDefaultLumaCoefs(SetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_setDefaultLumaCoefs(JNIEnv * env, jobject self, jfloatArray rgb)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->setDefaultLumaCoefs(GetJFloatArrayValue(env, rgb, "rgb", 3)());
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getLook(JNIEnv * env, jobject self, jstring name)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJConstObject<ConstLookRcPtr, LookJNI>(env, self,
             env->FindClass("org/OpenColorIO/Look"), cfg->getLook(GetJStringValue(env, name)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jint JNICALL
Java_org_OpenColorIO_Config_getNumLooks(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return (jint)cfg->getNumLooks();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jstring JNICALL
Java_org_OpenColorIO_Config_getLookNameByIndex(JNIEnv * env, jobject self, jint index)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return env->NewStringUTF(cfg->getLookNameByIndex((int)index));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_addLook(JNIEnv * env, jobject self, jobject look)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    ConstLookRcPtr lok = GetConstJOCIO<ConstLookRcPtr, LookJNI>(env, look);
    cfg->addLook(lok);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_Config_clearLooks(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConfigRcPtr cfg = GetEditableJOCIO<ConfigRcPtr, ConfigJNI>(env, self);
    cfg->clearLooks();
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_Context_2Lorg_OpenColorIO_ColorSpace_2Lorg_OpenColorIO_ColorSpace_2
  (JNIEnv * env, jobject self, jobject context, jobject srcColorSpace, jobject dstColorSpace)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, context);
    ConstColorSpaceRcPtr src = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, srcColorSpace);
    ConstColorSpaceRcPtr dst = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, dstColorSpace);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(con, src, dst));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_ColorSpace_2Lorg_OpenColorIO_ColorSpace_2
  (JNIEnv * env, jobject self, jobject srcColorSpace, jobject dstColorSpace)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstColorSpaceRcPtr src = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, srcColorSpace);
    ConstColorSpaceRcPtr dst = GetConstJOCIO<ConstColorSpaceRcPtr, ColorSpaceJNI>(env, dstColorSpace);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(src, dst));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv * env, jobject self, jstring srcName, jstring dstName)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(
               GetJStringValue(env, srcName)(), GetJStringValue(env, dstName)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_Context_2Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv * env, jobject self, jobject context, jstring srcName, jstring dstName)
{
    OCIO_JNITRY_ENTER()
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, context);
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(con,
               GetJStringValue(env, srcName)(), GetJStringValue(env, dstName)()));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_Transform_2
  (JNIEnv * env, jobject self, jobject transform)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(tran));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_Transform_2Lorg_OpenColorIO_TransformDirection_2
  (JNIEnv * env, jobject self, jobject transform, jobject direction)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    TransformDirection dir = GetJEnum<TransformDirection>(env, direction);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(tran, dir));
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_Config_getProcessor__Lorg_OpenColorIO_Context_2Lorg_OpenColorIO_Transform_2Lorg_OpenColorIO_TransformDirection_2
  (JNIEnv * env, jobject self, jobject context, jobject transform, jobject direction)
{
    OCIO_JNITRY_ENTER()
    ConstConfigRcPtr cfg = GetConstJOCIO<ConstConfigRcPtr, ConfigJNI>(env, self);
    ConstContextRcPtr con = GetConstJOCIO<ConstContextRcPtr, ContextJNI>(env, context);
    ConstTransformRcPtr tran = GetConstJOCIO<ConstTransformRcPtr, TransformJNI>(env, transform);
    TransformDirection dir = GetJEnum<TransformDirection>(env, direction);
    return BuildJConstObject<ConstProcessorRcPtr, ProcessorJNI>(env, self,
             env->FindClass("org/OpenColorIO/Processor"), cfg->getProcessor(con, tran, dir));
    OCIO_JNITRY_EXIT(NULL)
}
