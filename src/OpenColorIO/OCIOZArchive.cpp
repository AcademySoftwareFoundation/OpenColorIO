// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.
#include <OpenColorIO/OpenColorIO.h>
#if OCIO_ARCHIVE_SUPPORT

#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <limits>

#include <pystring.h>


#include "Mutex.h"
#include "Platform.h"
#include "utils/StringUtils.h"
#include "transforms/FileTransform.h"

#include "OCIOZArchive.h"

#include "mz.h"
#include "mz_os.h"
#include "mz_strm.h"
#include "mz_strm_buf.h"
#include "mz_strm_mem.h"
#include "mz_strm_os.h"
#include "mz_strm_split.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"

namespace OCIO_NAMESPACE
{
// Minizip-np compression levels.
enum ArchiveCompressionLevels
{
    DEFAULT = -1,
    FAST    = 2,
    NORMAL  = 6,
    BEST    = 9
};

// Minizip-np compression methods.
enum ArchiveCompressionMethods
{
    DEFLATE = 8
};

// Minizip-ng options for archive.
struct ArchiveOptions {
    uint8_t     include_path    = 0;
    int16_t     compress_level  = ArchiveCompressionLevels::BEST;
    uint8_t     compress_method = ArchiveCompressionMethods::DEFLATE;
    uint8_t     overwrite       = 0;
    uint8_t     append          = 0;
    int64_t     disk_size       = 0;
    uint8_t     follow_links    = 0;
    uint8_t     store_links     = 0;
    uint8_t     zip_cd          = 0;
    int32_t     encoding        = 0;
    uint8_t     verbose         = 0;
    uint8_t     aes             = 0;
    const char* cert_path;
    const char* cert_pwd;
};

/**
 * \brief Guard against early throws with Minizip-ng objects. 
 * 
 */
struct MinizipNgHandlerGuard
{
    MinizipNgHandlerGuard(void *& handle, bool isWriter, bool usingEntry) 
    : m_handle(handle), m_isWriter(isWriter), m_usingEntry(usingEntry)
    {
        // Nothing to do.
    }

    ~MinizipNgHandlerGuard()
    {
        if (m_handle != nullptr)
        {
            if (m_isWriter)
            {
                // Clean up writer object.
                if (m_usingEntry)
                {
                    // Clean up writer entry object.
                    mz_zip_writer_entry_close(m_handle);
                }
                // Close and delete the handle.
                mz_zip_writer_delete(&m_handle);
            }
            else
            {
                // Clean up reader object.
                if (m_usingEntry)
                {
                    // Clean up reader entry object.
                    mz_zip_reader_entry_close(m_handle);
                }

                // Close and delete the handle.
                mz_zip_reader_delete(&m_handle);
            }
            m_handle = nullptr;
        }
    }

    // Minizip-ng handler
    void *& m_handle;
    bool m_isWriter;
    bool m_usingEntry;
};

struct MinizipNgMemStreamGuard
{
    MinizipNgMemStreamGuard(void *& memStream) 
    : m_memStream(memStream)
    {
        // Nothing to do.
    }

    ~MinizipNgMemStreamGuard()
    {
        if (m_memStream != nullptr)
        {
            // Clean up memory stream.
            mz_stream_mem_close(m_memStream);
            mz_stream_mem_delete(&m_memStream); 
            m_memStream = nullptr;
        }
    }

    // Minizip-ng memory stream object.
    void *& m_memStream;
};

//////////////////////////////////////////////////////////////////////////////////////
// Utility functions section
//////////////////////////////////////////////////////////////////////////////////////
/**
 * Utility function for archived Configs.
 * 
 * \param archiver Minizip-ng handle object.
 * \param path Path of the file or the folder to add inside the OCIOZ archive.
 * \param configWorkingDirectory Working directory of the current config.
 */
void addSupportedFiles(void * archiver, const char * path, const char * configWorkingDirectory)
{
    DIR *dir = mz_os_open_dir(path);
    if (dir != NULL)
    {
        struct dirent *entry = NULL;
        while ((entry = mz_os_read_dir(dir)) != NULL) 
        {
            // Join the current path and the directory/file name to get the absolute path.
            std::string absPath = pystring::os::path::join(path, entry->d_name);
            // Check if the absolute path is a directory.
            if (mz_os_is_dir(absPath.c_str()) == MZ_OK)
            {
                // Since mz_os_read_dir is listing the whole directory, "." and ".." must be
                // ignored.
                if (!StringUtils::Compare(".", entry->d_name) && 
                    !StringUtils::Compare("..", entry->d_name))
                {
                    // Add the current directory.
                    addSupportedFiles(archiver, absPath.c_str(), configWorkingDirectory);
                }
            }
            else
            {
                // Absolute path is a file.
                std::string root, ext;
                pystring::os::path::splitext(root, ext, std::string(entry->d_name));
                // Strip leading dot character in order to get the extension name only.
                ext = pystring::lstrip(ext, ".");
                
                // Check if the extension is supported. Using logic from LoadFileUncached().
                FormatRegistry & formatRegistry = FormatRegistry::GetInstance();
                FileFormatVector possibleFormats;
                formatRegistry.getFileFormatForExtension(ext, possibleFormats);
                if (!possibleFormats.empty())
                {
                    // The extension is supported. Add the current file to the OCIOZ archive.
                    if (mz_zip_writer_add_path(
                        archiver, absPath.c_str(), 
                        configWorkingDirectory, 0, 1) != MZ_OK)
                    {
                        // Close DIR object before throwing.
                        mz_os_close_dir(dir);

                        std::ostringstream os;
                        os << "Could not write LUT file " << absPath << " to in-memory archive.";
                        throw Exception(os.str().c_str());
                    }
                }
            }
        }
        mz_os_close_dir(dir);
    }
}
//////////////////////////////////////////////////////////////////////////////////////

void archiveConfig(std::ostream & ostream, const Config & config, const char * configWorkingDirectory)
{
    void * archiver = nullptr;
    void *write_mem_stream = NULL;
    const uint8_t *buffer_ptr = NULL;
    int32_t buffer_size = 0;
    mz_zip_file file_info;

    if (!config.isArchivable())
    {
        std::ostringstream os;
        os << "Config is not archivable.";
        throw Exception(os.str().c_str());  
    }

    // Initialize.
    memset(&file_info, 0, sizeof(file_info));

    // Retrieve and store the config as string.
    std::stringstream ss;
    config.serialize(ss);
    std::string configStr = ss.str();

    // Write zip to memory stream.
#if MZ_VERSION_BUILD >= 040000
    write_mem_stream = mz_stream_mem_create();
#else
    mz_stream_mem_create(&write_mem_stream);
#endif
    mz_stream_mem_set_grow_size(write_mem_stream, 128 * 1024);
    mz_stream_open(write_mem_stream, NULL, MZ_OPEN_MODE_CREATE);

    // Minizip archive options.
    ArchiveOptions options;
    // Make sure that the compression method is set to DEFLATE.
    options.compress_method = ArchiveCompressionMethods::DEFLATE;
    // Make sure that the compression level is set to BEST.
    options.compress_level  = ArchiveCompressionLevels::BEST;

    // Create the writer handle.
#if MZ_VERSION_BUILD >= 040000
    archiver = mz_zip_writer_create();
#else
    mz_zip_writer_create(&archiver);
#endif

    // Archive options.
    // Compression method
    mz_zip_writer_set_compress_method(archiver, options.compress_method);
    // Compress level.
    mz_zip_writer_set_compress_level(archiver, options.compress_level);

    MinizipNgMemStreamGuard memStreamGuard(write_mem_stream);
    MinizipNgHandlerGuard archiverGuard(archiver, true, true);

    // Open the in-memory zip.
    if (mz_zip_writer_open(archiver, write_mem_stream, 0) == MZ_OK)
    {
        // Use a hardcoded name for the config's filename inside the archive.
        std::string configFullname = std::string(OCIO_CONFIG_DEFAULT_NAME) + 
                                     std::string(OCIO_CONFIG_DEFAULT_FILE_EXT);

        // Get config string size.
        int32_t configSize = (int32_t) configStr.size();

        // Initialize the config file information structure.
        file_info.filename = configFullname.c_str();
        file_info.modified_date = time(NULL);
        file_info.version_madeby = MZ_VERSION_MADEBY;
        file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        file_info.flag = MZ_ZIP_FLAG_UTF8;
        file_info.uncompressed_size = configSize;

        //////////////////////////////
        // Adding config to archive //
        //////////////////////////////
        int32_t written = 0;
        // Opens an entry in the in-memory zip file for writing.
        if (mz_zip_writer_entry_open(archiver, &file_info) == MZ_OK)
        {
            // Add config to the in-memory zip using buffer.
            written = mz_zip_writer_entry_write(archiver, configStr.c_str(), configSize);
            if (written < MZ_OK)
            {
                std::ostringstream os;
                os << "Could not write config to in-memory archive.";
                throw Exception(os.str().c_str());
            }
            // Close the entry.
            mz_zip_writer_entry_close(archiver);
        }
        else
        {
            std::ostringstream os;
            os << "Could not prepare an entry for writing.";
            throw Exception(os.str().c_str());
        }

        ///////////////////////
        // Adding LUT files //
        ///////////////////////
        // Add all supported files to in-memory zip from any directories under working directory. 
        // (recursive)
        addSupportedFiles(archiver, configWorkingDirectory, configWorkingDirectory);

        // Close in-memory zip.
        mz_zip_writer_close(archiver);
    }

    // Clean up.
    mz_zip_writer_delete(&archiver);
    
    // Get the buffer to write to the ostream.
    mz_stream_mem_get_buffer(write_mem_stream, (const void **)&buffer_ptr);
    mz_stream_mem_seek(write_mem_stream, 0, MZ_SEEK_END);
    buffer_size = (int32_t)mz_stream_mem_tell(write_mem_stream);

    ostream.write((char*)&buffer_ptr[0], buffer_size);  

    mz_stream_mem_close(write_mem_stream);
    mz_stream_mem_delete(&write_mem_stream); 
}

/**
 * \brief Extract the specified OCIOZ archive.
 * 
 * This function can only be used with the OCIOZ archive format (not arbitrary zip files).
 * 
 * Note: Signature is in OpenColorIO.h since the function is OCIOEXPORT-ed for client apps.
 */
void ExtractOCIOZArchive(const char * archivePath, const char * destination)
{
    void * extracter = NULL;
    int32_t err = MZ_OK;

    // Normalize the path for the platform.
    std::string outputDestination = pystring::os::path::normpath(destination);

    // Create zip reader.
#if MZ_VERSION_BUILD >= 040000
    extracter = mz_zip_reader_create();
#else
    mz_zip_reader_create(&extracter);
#endif

    MinizipNgHandlerGuard extracterGuard(extracter, false, false);

    // Open the OCIOZ archive.
    if (mz_zip_reader_open_file(extracter, archivePath) != MZ_OK) 
    {
        std::ostringstream os;
        os << "Could not open " << archivePath << " for reading.";
        throw Exception(os.str().c_str());
    } 
    else 
    {
        // Extract all entries to outputDestination directory.
        err = mz_zip_reader_save_all(extracter, outputDestination.c_str());
        if (err == MZ_END_OF_LIST) 
        {
            // The archive has no files.
            std::ostringstream os;
            os << "No files in archive.";
            throw Exception(os.str().c_str());
        } 
        else if (err != MZ_OK) 
        {
            std::ostringstream os;
            os << "Could not extract: " << archivePath;
            throw Exception(os.str().c_str());
        }
    }

    // Close the OCIOZ archive.
    if (mz_zip_reader_close(extracter) != MZ_OK) 
    {
        std::ostringstream os;
        os << "Could not close " << archivePath << " after reading.";
        throw Exception(os.str().c_str());
    }

    // Clean up.
    mz_zip_reader_delete(&extracter);
}

/**
 * \brief Callback function for getFileStringFromArchiveStream in order to get the contents of a
 *        file inside an OCIOZ archive as a buffer. 
 * 
 * The file is retrieved by comparing the paths.
 * 
 * \param reader Minizip-ng handle object.
 * \param info File information.
 * \param filepath Path to find.
 * 
 * \return Vector of uint8 with the content of the specified file from an OCIOZ archive.
 */
std::vector<uint8_t> getFileBufferByPath(void * reader, mz_zip_file & info, std::string filepath)
{
    // Verify that the file information and the current file matches while ignoring the slashes
    // differences in platforms.
    std::vector<uint8_t> buffer;
    if (mz_path_compare_wc(filepath.c_str(), info.filename, 1) == MZ_OK)
    {
        // Initialize the buffer for the file.
        int32_t buf_size = (int32_t)mz_zip_reader_entry_save_buffer_length(reader);
        buffer.resize(buf_size);
        // Read the content of the file and return it as buffer.
        mz_zip_reader_entry_save_buffer(reader, &buffer[0], buf_size);
    }
    return buffer;
}

/**
 * \brief Callback function for getFileStringFromArchiveStream in order to Get the content of a 
 *        file inside an OCIOZ archive as a buffer. 
 * 
 * The file is retrieved by comparing only the extension.  (Used for the .ocio file.)
 * 
 * \param reader Minizip-ng reader object.
 * \param info File information
 * \param extension Extension to find
 * 
 * \return Vector of uint8 with the content of the file from an OCIOZ archive.
 */
std::vector<uint8_t> getFileBufferByExtension(void * reader, mz_zip_file & info, std::string extension)
{
    std::string root, ext;
    std::vector<uint8_t> buffer;
    pystring::os::path::splitext(root, ext, info.filename);
    if (Platform::Strcasecmp(extension.c_str(), ext.c_str()) == 0)
    {
        int32_t buf_size = (int32_t)mz_zip_reader_entry_save_buffer_length(reader);
        buffer.resize(buf_size);
        mz_zip_reader_entry_save_buffer(reader, &buffer[0], buf_size);
    }
    return buffer;
}

/**
 * \brief Get the content of a file inside an OCIOZ archive as a buffer. 
 * 
 * The two possible callbacks are defined above:
 * getFileBufferByPath and getFileBufferByExtension.
 * 
 * \param filepath File to retrieve from the OCIOZ archive.
 * \param archivePath Path to the archive.
 * \param fn Callback function to get the (file) buffer by path or by extension.
 * 
 * \return Vector of uint8 with the content of the specified file from an OCIOZ archive.
 */
std::vector<uint8_t> getFileStringFromArchiveFile(const std::string & filepath, 
                    const std::string & archivePath, 
                    std::vector<uint8_t> (*fn)(void*, mz_zip_file&, std::string))
{
    mz_zip_file *file_info = NULL;
    int32_t err = MZ_OK;
    void *reader = NULL;
    std::vector<uint8_t> buffer;

    // Create the reader object.
#if MZ_VERSION_BUILD >= 040000
    reader = mz_zip_reader_create();
#else
    mz_zip_reader_create(&reader);
#endif

    MinizipNgHandlerGuard extracterGuard(reader, false, true);

    // Open the OCIOZ archive from the filesystem.
    err = mz_zip_reader_open_file(reader, archivePath.c_str());
    if (err != MZ_OK)
    {
        std::ostringstream os;
        os << "Could not open " << archivePath.c_str() 
           << " in order to get the file: " << filepath;
        throw Exception(os.str().c_str());
    }
    else
    {
        // Seek to the first entry in the OCIOZ archive.
        if (mz_zip_reader_goto_first_entry(reader) == MZ_OK)
        {
            do
            {
                // Get the current entry information.
                if (mz_zip_reader_entry_get_info(reader, &file_info) == MZ_OK)
                {
                    buffer = fn(reader, *file_info, filepath);
                    if (!buffer.empty()) 
                    {
                        break;
                    }
                }
            } while (mz_zip_reader_goto_next_entry(reader) == MZ_OK);  
        }
    }

    return buffer;
}

//////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////
// API section
//////////////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> getFileBufferFromArchive(const std::string & filepath, const std::string & archivePath)
{
    return getFileStringFromArchiveFile(filepath, archivePath, &getFileBufferByPath);
}

std::vector<uint8_t> getFileBufferFromArchiveByExtension(const std::string & extension, const std::string & archivePath)
{
    return getFileStringFromArchiveFile(extension, archivePath, &getFileBufferByExtension);
}

void getEntriesMappingFromArchiveFile(const std::string & archivePath, 
                                      std::map<std::string, std::string> & map)
{
    mz_zip_file *file_info = NULL;
    int32_t err = MZ_OK;
    void *reader = NULL;

    // Create the reader object.
#if MZ_VERSION_BUILD >= 040000
    reader = mz_zip_reader_create();
#else
    mz_zip_reader_create(&reader);
#endif

    MinizipNgHandlerGuard extracterGuard(reader, false, false);

    // Open the zip from file.
    err = mz_zip_reader_open_file(reader, archivePath.c_str());
    if (err != MZ_OK) 
    {
        std::ostringstream os;
        os << "Could not open " << archivePath.c_str() << " in order to get the entries.";
        throw Exception(os.str().c_str());
    }
    else
    {
        // Seek to the first entry in the OCIOZ archive.
        if (mz_zip_reader_goto_first_entry(reader) == MZ_OK)
        {
            do
            {
                // Get the current entry information.
                if (mz_zip_reader_entry_get_info(reader, &file_info) == MZ_OK)
                {
                    // file_info->filename is the complete path of the file from the root of the
                    // archive.
                    map.insert(
                        std::pair<std::string, std::string>(
                            file_info->filename, 
                            std::string(file_info->filename) + std::to_string(file_info->crc)
                        )
                    );
                }
            } while (mz_zip_reader_goto_next_entry(reader) == MZ_OK);  
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////
// Implementation of CIOPOciozArchive class.
//////////////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> CIOPOciozArchive::getLutData(const char * filepath) const
{
    // In order to ease the implementation and to facilitate a future Python binding, this method
    // uses std::vector<uint8_t> buffer instead of a std::istream.
    //
    // It is expected that in most cases, the compiler will be able to avoid copying the buffer
    // (RVO/NRVO).
    // 
    // During testings with ocioperf, the first iteration was a little slower using the buffer
    // instead of a std::istream (max 5%). But the following iterations are just as fast due to
    // the FileTransform cache.

    std::vector<uint8_t> buffer;
    buffer = getFileBufferFromArchive(pystring::os::path::normpath(filepath), m_archiveAbsPath);
    return buffer;
}

std::string CIOPOciozArchive::getConfigData() const
{
    // In order to ease the implementation and to facilitate a future Python binding, this method
    // returns a std::string instead of a std::istream.
    // 
    // It is expected that in most cases, the compiler will be able to avoid copying the buffer
    // (RVO/NRVO).
    std::string configData = "";
    std::string configFilename = std::string(OCIO_CONFIG_DEFAULT_NAME) +
                                 std::string(OCIO_CONFIG_DEFAULT_FILE_EXT);
    std::vector<uint8_t> configBuffer = getFileBufferFromArchive(configFilename, m_archiveAbsPath);
    if (configBuffer.size() > 0)
    {
        configData = std::string(configBuffer.begin(), configBuffer.end());
    }

    return configData;
}

std::string CIOPOciozArchive::getFastLutFileHash(const char * filepath) const
{
    std::string hash = "";
    // Check into the map to check if the file exists in the archive.
    // The key is the full path of the file inside the archive and the value is the hash.
    // Normalize filepath and find it in the std::map.
    std::string fpath = pystring::os::path::normpath(filepath);
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        // Verify that the key and the specfied filepath matches while ignoring the slashes
        // differences in platforms.
        if (mz_path_compare_wc(it->first.c_str(), fpath.c_str(), 1) == MZ_OK)
        {
            hash = std::string(it->second);
        }
    }
    return hash;
}

void CIOPOciozArchive::setArchiveAbsPath(const std::string & absPath)
{
    m_archiveAbsPath = absPath;
}

void CIOPOciozArchive::buildEntries()
{
    std::ifstream ociozStream = Platform::CreateInputFileStream(
        m_archiveAbsPath.c_str(), 
        std::ios_base::in | std::ios_base::binary
    );

    if (ociozStream.fail())
    {
        std::ostringstream os;
        os << "Error could not read OCIOZ archive: " << m_archiveAbsPath;
        throw Exception (os.str().c_str());
    }

    getEntriesMappingFromArchiveFile(m_archiveAbsPath, m_entries);
}

} // namespace OCIO_NAMESPACE

#endif //OCIO_ARCHIVE_SUPPORT