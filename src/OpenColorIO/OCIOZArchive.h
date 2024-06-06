// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_ARCHIVEUTILS_H
#define INCLUDED_OCIO_ARCHIVEUTILS_H

#include <OpenColorIO/OpenColorIO.h>

#if OCIO_ARCHIVE_SUPPORT

#include <sstream>
#include <fstream>
#include <vector>
#include <map>
#include <string>


namespace OCIO_NAMESPACE
{
/**
 * \brief Archive a config into an OCIOZ file.
 * 
 * Note: The config file inside the archive is hardcoded to "config.ocio".
 * 
 * \param ostream Output stream to write the data into.
 * \param config Config object.
 * \param configWorkingDirectory Working directory of the current config.
 */
void archiveConfig(
    std::ostream & ostream, 
    const Config & config, 
    const char * configWorkingDirectory);

/**
 * \brief Get the content of a file inside an OCIOZ archive as a buffer. 
 * 
 * The file is retrieve by comparing the paths.
 * 
 * \param filepath Path to find.
 * \param archivePath Path to archive
 * 
 * \return Vector of uint8 with the content of the specified file from an OCIOZ archive.
 */
std::vector<uint8_t> getFileBufferFromArchive(
    const std::string & filepath, 
    const std::string & archivePath);

/**
 * \brief Get the content of a file inside an OCIOZ archive as a buffer. 
 * 
 * The file is retrieve by comparing the extensions.
 * 
 * \param extension Extension to find
 * \param archivePath Path to archive
 * 
 * \return Vector of uint8 with the content of the specified file from an OCIOZ archive.
 */
std::vector<uint8_t> getFileBufferFromArchiveByExtension(
    const std::string & extension, 
    const std::string & archivePath);

/**
 * \brief Get the Entries from OCIOZ archive
 * 
 * Populate a std::map object with the following information:
 * key => value : full_path_of_the_file_inside_archive => calculated_hash_of_the_file
 * 
 * The hash is calculated using the full path of the file inside the archive and its CRC32.
 * 
 * \param archivePath Path to archive.
 * \param map std::map object to be populated
 */
void getEntriesMappingFromArchiveFile(
    const std::string & archivePath, 
    std::map<std::string, std::string> & map);

//////////////////////////////////////////////////////////////////////////////////////

class CIOPOciozArchive : public ConfigIOProxy
{
public:
    CIOPOciozArchive() = default;

    // See OpenColorIO.h for informations on these five methods.
    
    std::vector<uint8_t> getLutData(const char * filepath) const override;
    std::string getConfigData() const override;
    // Currently using the filepath of the file + the CRC32.
    std::string getFastLutFileHash(const char * filepath) const override;

    // Following methods are specific to CIOPOciozArchive.
    /**
     * \brief Set the OCIOZ archive absolute path.
     * 
     * \param absPath Absolute path to OCIOZ archive.
     */
    void setArchiveAbsPath(const std::string & absPath);

    /**
     * \brief Build a map of the zip file table of contents for the files in the archive.
     * 
     * The structure is a std::map with the key as the full path of the file and the value as a 
     * calculated hash.
     */
    void buildEntries();
private:
    std::string m_archiveAbsPath;
    std::map<std::string, std::string> m_entries;
};

} // namespace OCIO_NAMESPACE

#endif //OCIO_ARCHIVE_SUPPORT
#endif
