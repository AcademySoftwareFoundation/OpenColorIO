// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#include <sstream>

#include <OpenColorIO/OpenColorIO.h>

#include "ContextVariableUtils.h"
#include "HashUtils.h"
#include "OpBuilders.h"
#include "transforms/FileTransform.h"
#include "transforms/GroupTransform.h"


namespace OCIO_NAMESPACE
{

namespace
{
// ConfigIOProxy implementation that provides the contents of a single in-memory LUT file
// buffer, regardless of the file path being requested.
class BufferConfigIOProxy : public ConfigIOProxy
{
public:
    BufferConfigIOProxy(const char * buffer, size_t bufferSize, const std::string & hash)
        : m_buffer(reinterpret_cast<const uint8_t *>(buffer),
                   reinterpret_cast<const uint8_t *>(buffer) + bufferSize)
        , m_hash(hash)
    {
    }

    std::vector<uint8_t> getLutData(const char * /* filepath */) const override
    {
        return m_buffer;
    }

    std::string getConfigData() const override
    {
        // Unused, the config itself is not provided through the proxy.
        return "";
    }

    std::string getFastLutFileHash(const char * /* filepath */) const override
    {
        return m_hash;
    }

private:
    std::vector<uint8_t> m_buffer;
    std::string m_hash;
};
} // anonymous namespace

GroupTransformRcPtr GroupTransform::Create()
{
    return GroupTransformRcPtr(new GroupTransformImpl(), &GroupTransformImpl::Deleter);
}

GroupTransformRcPtr GroupTransform::ParseFromBuffer(const char * buffer, size_t bufferSize)
{
    if (!buffer || bufferSize == 0)
    {
        throw Exception("GroupTransform::ParseFromBuffer: buffer is null or empty.");
    }

    // Hash the buffer contents. The hash is used both to form the synthetic file name given
    // to the FileTransform and as the fast LUT file hash returned by the ConfigIOProxy, so
    // that the global file caches (which are keyed on these strings) never confuse the
    // contents of two different buffers.
    const std::string contentHash = CacheIDHash(buffer, bufferSize);

    ConfigIOProxyRcPtr ciop = std::make_shared<BufferConfigIOProxy>(buffer,
                                                                    bufferSize,
                                                                    contentHash);

    ConfigRcPtr config = Config::CreateRaw()->createEditableCopy();
    config->setConfigIOProxy(ciop);

    // Prefix the hash to form a synthetic file name so that the global file cache entries
    // created here can never collide with those of a real file whose resolved path happens
    // to match the bare hash string.
    const std::string syntheticFileName = "ParseFromBuffer:" + contentHash;

    FileTransformRcPtr fileTransform = FileTransform::Create();
    fileTransform->setSrc(syntheticFileName.c_str());

    try
    {
        ConstProcessorRcPtr processor = config->getProcessor(fileTransform);
        return processor->createGroupTransform();
    }
    catch (Exception & e)
    {
        std::ostringstream os;
        os << "GroupTransform::ParseFromBuffer: Error parsing LUT from buffer: " << e.what();
        throw Exception(os.str().c_str());
    }
}

void GroupTransformImpl::Deleter(GroupTransform * t)
{
    delete static_cast<GroupTransformImpl *>(t);
}


///////////////////////////////////////////////////////////////////////////

GroupTransformImpl::GroupTransformImpl()
    : m_metadata()
    , m_dir(TRANSFORM_DIR_FORWARD)
{
}

TransformRcPtr GroupTransformImpl::createEditableCopy() const
{
    GroupTransformRcPtr transform = GroupTransform::Create();
    auto groupImpl = dynamic_cast<GroupTransformImpl*>(transform.get());
    groupImpl->m_dir = m_dir;
    groupImpl->m_vec = m_vec;
    groupImpl->m_metadata = m_metadata;
    return transform;
}

TransformDirection GroupTransformImpl::getDirection() const noexcept
{
    return m_dir;
}

void GroupTransformImpl::setDirection(TransformDirection dir) noexcept
{
    m_dir = dir;
}

void GroupTransformImpl::validate() const
{
    try
    {
        Transform::validate();
    }
    catch (Exception & ex)
    {
        std::string errMsg("GroupTransform validation failed: ");
        errMsg += ex.what();
        throw Exception(errMsg.c_str());
    }

    for(const auto & val : m_vec)
    {
        val->validate();
    }
}

int GroupTransformImpl::getNumTransforms() const noexcept
{
    return static_cast<int>(m_vec.size());
}

ConstTransformRcPtr GroupTransformImpl::getTransform(int index) const
{
    if (index < 0 || index >= (int)m_vec.size())
    {
        std::ostringstream os;
        os << "Invalid transform index " << index << ".";
        throw Exception(os.str().c_str());
    }

    return m_vec[index];
}

TransformRcPtr & GroupTransformImpl::getTransform(int index)
{
    if (index < 0 || index >= (int)m_vec.size())
    {
        std::ostringstream os;
        os << "Invalid transform index " << index << ".";
        throw Exception(os.str().c_str());
    }

    return m_vec[index];
}

void GroupTransformImpl::appendTransform(TransformRcPtr transform) noexcept
{
    m_vec.push_back(transform);
}

void GroupTransformImpl::prependTransform(TransformRcPtr transform) noexcept
{
    m_vec.insert(m_vec.begin(), transform);
}

void GroupTransformImpl::write(const ConstConfigRcPtr & config,
                               const char * formatName,
                               std::ostream & os) const
{
    FileFormat* fmt = FormatRegistry::GetInstance().getFileFormatByName(formatName);

    if (!fmt)
    {
        std::ostringstream err;
        err << "The format named '" << formatName;
        err << "' could not be found. ";
        throw Exception(err.str().c_str());
    }

    try
    {
        fmt->write(config, config->getCurrentContext(), *this, formatName, os);
    }
    catch (std::exception & e)
    {
        std::ostringstream err;
        err << "Error writing format '" << formatName << "': ";
        err << e.what();
        throw Exception(err.str().c_str());
    }
}

int GroupTransform::GetNumWriteFormats() noexcept
{
    return FormatRegistry::GetInstance().getNumFormats(FORMAT_CAPABILITY_WRITE);
}

const char * GroupTransform::GetFormatNameByIndex(int index) noexcept
{
    return FormatRegistry::GetInstance().getFormatNameByIndex(FORMAT_CAPABILITY_WRITE, index);
}

const char * GroupTransform::GetFormatExtensionByIndex(int index) noexcept
{
    return FormatRegistry::GetInstance().getFormatExtensionByIndex(FORMAT_CAPABILITY_WRITE, index);
}

std::ostream & operator<< (std::ostream & os, const GroupTransform & groupTransform)
{
    os << "<GroupTransform ";
    os << "direction=" << TransformDirectionToString(groupTransform.getDirection()) << ", ";
    os << "transforms=";
    for (int i = 0; i < groupTransform.getNumTransforms(); ++i)
    {
        ConstTransformRcPtr transform = groupTransform.getTransform(i);
        os << "\n        " << *transform;
    }
    os << ">";
    return os;
}

///////////////////////////////////////////////////////////////////////////

void BuildGroupOps(OpRcPtrVec & ops,
                    const Config & config,
                    const ConstContextRcPtr & context,
                    const GroupTransform & groupTransform,
                    TransformDirection dir)
{
    if (ops.size() == 0)
    {
        // If group is the first transform, copy the group metadata.
        FormatMetadataImpl & processorData = ops.getFormatMetadata();
        processorData = groupTransform.getFormatMetadata();
    }

    auto combinedDir = CombineTransformDirections(dir, groupTransform.getDirection());

    switch (combinedDir)
    {
    case TRANSFORM_DIR_FORWARD:
        for (int i = 0; i < groupTransform.getNumTransforms(); ++i)
        {
            ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
            BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_FORWARD);
        }
        break;
    case TRANSFORM_DIR_INVERSE:
        for (int i = groupTransform.getNumTransforms() - 1; i >= 0; --i)
        {
            ConstTransformRcPtr childTransform = groupTransform.getTransform(i);
            BuildOps(ops, config, context, childTransform, TRANSFORM_DIR_INVERSE);
        }
        break;
    }
}

bool CollectContextVariables(const Config & config,
                             const Context & context,
                             const GroupTransform & tr,
                             ContextRcPtr & usedContextVars)
{
    bool foundContextVars = false;

    for (int idx = 0; idx < tr.getNumTransforms(); ++idx)
    {
        ConstTransformRcPtr child = tr.getTransform(idx);
        if (CollectContextVariables(config, context, child, usedContextVars))
        {
            foundContextVars = true;
        }
    }

    return foundContextVars;
}

} // namespace OCIO_NAMESPACE

