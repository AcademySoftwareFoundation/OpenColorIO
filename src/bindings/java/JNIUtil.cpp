// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <OpenColorIO/OpenColorIO.h>

#include "JNIUtil.h"

namespace OCIO_NAMESPACE
{

jobject NewJFloatBuffer(JNIEnv * env, float* ptr, int32_t len) {
    jobject byte_buf = env->NewDirectByteBuffer(ptr, len * sizeof(float));
    jmethodID mid = env->GetMethodID(env->GetObjectClass(byte_buf),
                                     "asFloatBuffer", "()Ljava/nio/FloatBuffer;");
    if (mid == 0) throw Exception("Could not find asFloatBuffer() method");
    return env->CallObjectMethod(byte_buf, mid);
}

float* GetJFloatBuffer(JNIEnv * env, jobject buffer, int32_t len) {
    jmethodID mid = env->GetMethodID(env->GetObjectClass(buffer), "isDirect", "()Z");
    if (mid == 0) throw Exception("Could not find isDirect() method");
    if(!env->CallBooleanMethod(buffer, mid)) {
        std::ostringstream err;
        err << "the FloatBuffer object is not 'direct' it needs to be created ";
        err << "from a ByteBuffer.allocateDirect(..).asFloatBuffer() call.";
        throw Exception(err.str().c_str());
    }
    if(env->GetDirectBufferCapacity(buffer) != len) {
        std::ostringstream err;
        err << "the FloatBuffer object is not allocated correctly it needs to ";
        err << "of size " << len << " but is ";
        err << env->GetDirectBufferCapacity(buffer) << ".";
        throw Exception(err.str().c_str());
    }
    return (float*)env->GetDirectBufferAddress(buffer);
}

const char* GetOCIOTClass(ConstTransformRcPtr tran) {
    if(ConstAllocationTransformRcPtr at = DynamicPtrCast<const AllocationTransform>(tran))
        return "org/OpenColorIO/AllocationTransform";
    else if(ConstCDLTransformRcPtr ct = DynamicPtrCast<const CDLTransform>(tran))
        return "org/OpenColorIO/CDLTransform";
    else if(ConstColorSpaceTransformRcPtr cst = DynamicPtrCast<const ColorSpaceTransform>(tran))
        return "org/OpenColorIO/ColorSpaceTransform";
    else if(ConstDisplayTransformRcPtr dt = DynamicPtrCast<const DisplayTransform>(tran))
        return "org/OpenColorIO/DisplayTransform";
    else if(ConstExponentTransformRcPtr et = DynamicPtrCast<const ExponentTransform>(tran))
        return "org/OpenColorIO/ExponentTransform";
    else if(ConstFileTransformRcPtr ft = DynamicPtrCast<const FileTransform>(tran))
        return "org/OpenColorIO/FileTransform";
    else if(ConstGroupTransformRcPtr gt = DynamicPtrCast<const GroupTransform>(tran))
        return "org/OpenColorIO/GroupTransform";
    else if(ConstLogTransformRcPtr lt = DynamicPtrCast<const LogTransform>(tran))
        return "org/OpenColorIO/LogTransform";
    else if(ConstLookTransformRcPtr lkt = DynamicPtrCast<const LookTransform>(tran))
        return "org/OpenColorIO/LookTransform";
    else if(ConstMatrixTransformRcPtr mt = DynamicPtrCast<const MatrixTransform>(tran))
        return "org/OpenColorIO/MatrixTransform";
    else return "org/OpenColorIO/Transform";
}

void JNI_Handle_Exception(JNIEnv * env)
{
    try
    {
        throw;
    }
    catch (ExceptionMissingFile & e)
    {
        jclass je = env->FindClass("org/OpenColorIO/ExceptionMissingFile");
        env->ThrowNew(je, e.what());
        env->DeleteLocalRef(je);
    }
    catch (Exception & e)
    {
        jclass je = env->FindClass("org/OpenColorIO/ExceptionBase");
        env->ThrowNew(je, e.what());
        env->DeleteLocalRef(je);
    }
    catch (std::exception& e)
    {
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, e.what());
        env->DeleteLocalRef(je);
    }
    catch (...)
    {
        jclass je = env->FindClass("java/lang/Exception");
        env->ThrowNew(je, "Unknown C++ exception caught.");
        env->DeleteLocalRef(je);
    }
}

} // namespace OCIO_NAMESPACE
