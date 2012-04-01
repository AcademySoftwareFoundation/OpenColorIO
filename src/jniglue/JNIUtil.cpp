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

#include <OpenColourIO/OpenColourIO.h>

#include "JNIUtil.h"

OCIO_NAMESPACE_ENTER
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
        return "org/OpenColourIO/AllocationTransform";
    else if(ConstCDLTransformRcPtr ct = DynamicPtrCast<const CDLTransform>(tran))
        return "org/OpenColourIO/CDLTransform";
    else if(ConstColourSpaceTransformRcPtr cst = DynamicPtrCast<const ColourSpaceTransform>(tran))
        return "org/OpenColourIO/ColourSpaceTransform";
    else if(ConstDisplayTransformRcPtr dt = DynamicPtrCast<const DisplayTransform>(tran))
        return "org/OpenColourIO/DisplayTransform";
    else if(ConstExponentTransformRcPtr et = DynamicPtrCast<const ExponentTransform>(tran))
        return "org/OpenColourIO/ExponentTransform";
    else if(ConstFileTransformRcPtr ft = DynamicPtrCast<const FileTransform>(tran))
        return "org/OpenColourIO/FileTransform";
    else if(ConstGroupTransformRcPtr gt = DynamicPtrCast<const GroupTransform>(tran))
        return "org/OpenColourIO/GroupTransform";
    else if(ConstLogTransformRcPtr lt = DynamicPtrCast<const LogTransform>(tran))
        return "org/OpenColourIO/LogTransform";
    else if(ConstLookTransformRcPtr lkt = DynamicPtrCast<const LookTransform>(tran))
        return "org/OpenColourIO/LookTransform";
    else if(ConstMatrixTransformRcPtr mt = DynamicPtrCast<const MatrixTransform>(tran))
        return "org/OpenColourIO/MatrixTransform";
    else if(ConstTruelightTransformRcPtr tt = DynamicPtrCast<const TruelightTransform>(tran))
        return "org/OpenColourIO/TruelightTransform";
    else return "org/OpenColourIO/Transform";
}

void JNI_Handle_Exception(JNIEnv * env)
{
    try
    {
        throw;
    }
    catch (ExceptionMissingFile & e)
    {
        jclass je = env->FindClass("org/OpenColourIO/ExceptionMissingFile");
        env->ThrowNew(je, e.what());
        env->DeleteLocalRef(je);
    }
    catch (Exception & e)
    {
        jclass je = env->FindClass("org/OpenColourIO/ExceptionBase");
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

}
OCIO_NAMESPACE_EXIT
