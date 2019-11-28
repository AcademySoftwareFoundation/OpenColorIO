// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "OpenColorIO/OpenColorIO.h"
#include "OpenColorIOJNI.h"
#include "JNIUtil.h"
using namespace OCIO_NAMESPACE;

namespace
{
void ImageDesc_deleter(ImageDesc* d)
{
    delete d;
}

void ImageDesc_dispose(JNIEnv * env, jobject self)
{
    DisposeJOCIO<ImageDescJNI>(env, self);
}

}; // end anon namespace

// PackedImageDesc

JNIEXPORT void JNICALL
Java_org_OpenColorIO_PackedImageDesc_create__Ljava_nio_FloatBuffer_2JJJ(JNIEnv * env,
    jobject self, jobject data, jlong width, jlong height, jlong numChannels)
{
    OCIO_JNITRY_ENTER()
    float* _data = GetJFloatBuffer(env, data, width * height * numChannels);
    if(!_data) throw Exception("Could not find direct buffer address for data");
    ImageDescJNI * jnistruct = new ImageDescJNI();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new ConstImageDescRcPtr();
    jnistruct->cppobj = new ImageDescRcPtr();
    *jnistruct->cppobj = ImageDescRcPtr(new PackedImageDesc(_data, (long)width,
        (long)height, (long)numChannels), &ImageDesc_deleter);
    jnistruct->isconst = false;
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    env->SetLongField(self, fid, (jlong)jnistruct);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_PackedImageDesc_create__Ljava_nio_FloatBuffer_2JJJJJJ(JNIEnv * env,
    jobject self, jobject data, jlong width, jlong height, jlong numChannels,
    jlong chanStrideBytes, jlong xStrideBytes, jlong yStrideBytes)
{
    OCIO_JNITRY_ENTER()
    float* _data = GetJFloatBuffer(env, data, width * height * numChannels);
    if(!_data) throw Exception("Could not find direct buffer address for data");
    ImageDescJNI * jnistruct = new ImageDescJNI();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new ConstImageDescRcPtr();
    jnistruct->cppobj = new ImageDescRcPtr();
    *jnistruct->cppobj = ImageDescRcPtr(new PackedImageDesc(_data, (long)width,
        (long)height, (long)numChannels, (long)chanStrideBytes, (long) xStrideBytes,
        (long)yStrideBytes), &ImageDesc_deleter);
    jnistruct->isconst = false;
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    env->SetLongField(self, fid, (jlong)jnistruct);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_PackedImageDesc_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ImageDesc_dispose(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_PackedImageDesc_getData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    int size = ptr->getWidth() * ptr->getHeight() * ptr->getNumChannels();
    return NewJFloatBuffer(env, ptr->getData(), size);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getWidth(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getWidth();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getHeight(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getHeight();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getNumChannels(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getNumChannels();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getChanStrideBytes(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getChanStrideBytes();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getXStrideBytes(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getXStrideBytes();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PackedImageDesc_getYStrideBytes(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPackedImageDescRcPtr ptr = DynamicPtrCast<const PackedImageDesc>(img);
    return (jlong)ptr->getYStrideBytes();
    OCIO_JNITRY_EXIT(0)
}

// PlanarImageDesc

JNIEXPORT void JNICALL
Java_org_OpenColorIO_PlanarImageDesc_dispose(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ImageDesc_dispose(env, self);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void JNICALL
Java_org_OpenColorIO_PlanarImageDesc_create__Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2JJ
    (JNIEnv * env, jobject self, jobject rData, jobject gData, jobject bData,
    jobject aData, jlong width, jlong height)
{
    OCIO_JNITRY_ENTER()
    int32_t size = width * height;
    float* _rdata = GetJFloatBuffer(env, rData, size);
    if(!_rdata) throw Exception("Could not find direct buffer address for rData");
    float* _gdata = GetJFloatBuffer(env, gData, size);
    if(!_gdata) throw Exception("Could not find direct buffer address for gData");
    float* _bdata = GetJFloatBuffer(env, bData, size);
    if(!_bdata) throw Exception("Could not find direct buffer address for bData");
    float* _adata = GetJFloatBuffer(env, aData, size);
    if(!_adata) throw Exception("Could not find direct buffer address for aData");
    ImageDescJNI * jnistruct = new ImageDescJNI();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new ConstImageDescRcPtr();
    jnistruct->cppobj = new ImageDescRcPtr();
    *jnistruct->cppobj = ImageDescRcPtr(new PlanarImageDesc(_rdata, _gdata,
        _bdata, _adata, (long)width, (long)height), &ImageDesc_deleter);
    jnistruct->isconst = false;
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    env->SetLongField(self, fid, (jlong)jnistruct);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT void
JNICALL Java_org_OpenColorIO_PlanarImageDesc_create__Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2Ljava_nio_FloatBuffer_2JJJ
    (JNIEnv * env, jobject self, jobject rData, jobject gData, jobject bData,
    jobject aData, jlong width, jlong height, jlong yStrideBytes)
{
    OCIO_JNITRY_ENTER()
    int32_t size = width * height;
    float* _rdata = GetJFloatBuffer(env, rData, size);
    if(!_rdata) throw Exception("Could not find direct buffer address for rData");
    float* _gdata = GetJFloatBuffer(env, gData, size);
    if(!_gdata) throw Exception("Could not find direct buffer address for gData");
    float* _bdata = GetJFloatBuffer(env, bData, size);
    if(!_bdata) throw Exception("Could not find direct buffer address for bData");
    float* _adata = GetJFloatBuffer(env, aData, size);
    if(!_adata) throw Exception("Could not find direct buffer address for aData");
    ImageDescJNI * jnistruct = new ImageDescJNI();
    jnistruct->back_ptr = env->NewGlobalRef(self);
    jnistruct->constcppobj = new ConstImageDescRcPtr();
    jnistruct->cppobj = new ImageDescRcPtr();
    *jnistruct->cppobj = ImageDescRcPtr(new PlanarImageDesc(_rdata, _gdata, _bdata,
        _adata, (long)width, (long)height, (long)yStrideBytes), &ImageDesc_deleter);
    jnistruct->isconst = false;
    jclass wclass = env->GetObjectClass(self);
    jfieldID fid = env->GetFieldID(wclass, "m_impl", "J");
    env->SetLongField(self, fid, (jlong)jnistruct);
    OCIO_JNITRY_EXIT()
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getRData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    int size = ptr->getWidth() * ptr->getHeight();
    return NewJFloatBuffer(env, ptr->getRData(), size);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getGData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    int size = ptr->getWidth() * ptr->getHeight();
    return NewJFloatBuffer(env, ptr->getGData(), size);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getBData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    int size = ptr->getWidth() * ptr->getHeight();
    return NewJFloatBuffer(env, ptr->getBData(), size);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jobject JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getAData(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    int size = ptr->getWidth() * ptr->getHeight();
    return NewJFloatBuffer(env, ptr->getAData(), size);
    OCIO_JNITRY_EXIT(NULL)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getWidth(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    return (jlong)ptr->getWidth();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getHeight(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    return (jlong)ptr->getHeight();
    OCIO_JNITRY_EXIT(0)
}

JNIEXPORT jlong JNICALL
Java_org_OpenColorIO_PlanarImageDesc_getYStrideBytes(JNIEnv * env, jobject self)
{
    OCIO_JNITRY_ENTER()
    ConstImageDescRcPtr img = GetConstJOCIO<ConstImageDescRcPtr, ImageDescJNI>(env, self);
    ConstPlanarImageDescRcPtr ptr = DynamicPtrCast<const PlanarImageDesc>(img);
    return (jlong)ptr->getYStrideBytes();
    OCIO_JNITRY_EXIT(0)
}
