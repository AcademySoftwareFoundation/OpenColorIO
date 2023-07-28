// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <fstream>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
#include "utils/StringUtils.h"

namespace OCIO = OCIO_NAMESPACE;

#include "apputils/argparse.h"

// Config archive functionality.
#include "mz.h"
#include "mz_os.h"
#include "mz_strm.h"
#include "mz_zip.h"
#include "mz_zip_rw.h"

// Array of non OpenColorIO arguments.
static std::vector<std::string> args;

// Fill 'args' array with OpenColorIO arguments.
static int
parse_end_args(int argc, const char *argv[])
{
    while (argc>0)
    {
        args.push_back(argv[0]);
        argc--;
        argv++;
    }

    return 0;
}

int main(int argc, const char **argv)
{
    ArgParse ap;
    std::string inputconfig;
    std::string archiveName;
    std::string configFilename;
    // Default value is current directory.
    std::string extractDestination;

    bool extract    = false;
    bool list       = false;
    bool help       = false;

    int32_t err = MZ_OK;
    mz_zip_file* file_info = NULL;
    void* reader = NULL;

    ap.options("ocioarchive -- Archive a config and its LUT files or extract a config archive. \n\n"
               "    Note that any existing OCIOZ archive with the same name will be overwritten.\n"
               "    The .ocioz extension will be added to the archive name, if not provided.\n\n"
               "Usage:\n"
               "    # Archive from the OCIO environment variable into myarchive.ocioz\n"
               "    ocioarchive myarchive\n\n"
               "    # Archive myconfig/config.ocio into myarchive.ocioz\n"
               "    ocioarchive myarchive --iconfig myconfig/config.ocio\n\n"
               "    # Extract myarchive.ocioz into new directory named myarchive\n"
               "    ocioarchive --extract myarchive.ocioz\n\n"
               "    # Extract myarchive.ocioz into new directory named ocio_config\n"
               "    ocioarchive --extract myarchive.ocioz --dir ocio_config\n\n"
               "    # List the files inside myarchive.ocioz\n"
               "    ocioarchive --list myarchive.ocioz\n",
               "%*", parse_end_args, "",
               "<SEPARATOR>", "Options:",
               "--iconfig %s",  &configFilename,        "Config to archive (takes precedence over $OCIO)",
               "--extract",     &extract,               "Extract an OCIOZ config archive",
               "--dir %s",      &extractDestination,    "Path where to extract the files (folders are created if missing)",
               "--list",        &list,                  "List the files inside an archive without extracting it",
               "--help",        &help,                  "Display the help and exit",
               "-h",            &help,                  "Display the help and exit",
               NULL
               );

    if (ap.parse (argc, argv) < 0)
    {
        std::cerr << ap.geterror() << std::endl;
        exit(1);
    }

    if (help || args.size() == 0)
    {
        ap.usage();
        return 0;
    }

    // Archiving.

    if (!extract && !list)
    {
        if (args.size() != 1)
        {
            std::cerr << "ERROR: Missing the name of the archive to create." << std::endl;
            exit(1);
        }

        archiveName = args[0].c_str();
        try
        {
            const char * ocioEnv = OCIO::GetEnvVariable("OCIO");
            OCIO::ConstConfigRcPtr config;
            if (!configFilename.empty())
            {
                // Archive a config from a config file (e.g. /home/user/ocio/config.ocio).
                try
                {
                    config = OCIO::Config::CreateFromFile(configFilename.c_str());
                }
                catch (...)
                {
                    // Capture any errors and display a custom message.
                    std::cerr << "ERROR: Could not load config: " << configFilename << std::endl;
                    exit(1);
                }
                
            }
            else if (ocioEnv && *ocioEnv)
            {
                // Archive a config from the environment variable.
                std::cout << "Archiving $OCIO=" << ocioEnv << std::endl;
                try
                {
                    config = OCIO::Config::CreateFromEnv();
                }
                catch (...)
                {
                    // Capture any errors and display a custom message.
                    std::cerr << "ERROR: Could not load config from $OCIO variable: " 
                              << ocioEnv << std::endl;
                    exit(1);
                }
            }
            else
            {
                std::cerr << "ERROR: You must specify an input OCIO configuration." << std::endl;
                exit(1);
            }

            try 
            {
                // The ocioz extension is added by the archive method. The assumption is that
                // archiveName is the filename without extension.
        
                // Do not add ocioz extension if already present.
                if (!StringUtils::EndsWith(archiveName, ".ocioz"))
                {
                    archiveName += std::string(OCIO::OCIO_CONFIG_ARCHIVE_FILE_EXT);
                }

                std::ofstream ofstream(archiveName, std::ofstream::out | std::ofstream::binary);
                if (ofstream.good())
                {
                    config->archive(ofstream);
                    ofstream.close();
                }
                else
                {
                    std::cerr << "Could not open output stream for: " 
                              << archiveName + std::string(OCIO::OCIO_CONFIG_ARCHIVE_FILE_EXT)
                              << std::endl;
                    exit(1);
                }
            }
            catch (OCIO::Exception & e)
            {
                std::cerr << e.what() << std::endl;
                exit(1);
            } 
        }
        catch (OCIO::Exception & exception)
        {
            std::cerr << "ERROR: " << exception.what() << std::endl;
            exit(1);
        } 
        catch (std::exception& exception)
        {
            std::cerr << "ERROR: " << exception.what() << std::endl;
            exit(1);
        }
        catch (...)
        {
            std::cerr << "ERROR: Unknown problem encountered." << std::endl;
            exit(1);
        }
    }

    // Extracting.

    else if (extract && !list)
    {
        if (args.size() != 1)
        {
            std::cerr << "ERROR: Missing the name of the archive to extract." << std::endl;
            exit(1);
        }

        archiveName = args[0].c_str();
        try 
        {
            if (extractDestination.empty())
            {
                // Set the default directory name to the name of the archive.
                extractDestination = archiveName;

                // Remove extension, if present.
                char * dst = const_cast<char*>(extractDestination.c_str());
                mz_path_remove_extension(dst);
                extractDestination = dst;
            }

            OCIO::ExtractOCIOZArchive(archiveName.c_str(), extractDestination.c_str());
            std::cout << archiveName << " has been extracted." << std::endl;
        }
        catch (OCIO::Exception & e)
        {
            std::cerr << e.what() << std::endl;
            exit(1);
        }
    }

    // Listing.

    else if (list && !extract)
    {
        if (args.size() < 1)
        {
            std::cerr << "ERROR: Missing the name of the archive to list." << std::endl;
            exit(1);
        }

        std::string path = args[0];
#if MZ_VERSION_BUILD >= 040000
        reader = mz_zip_reader_create();
#else
        mz_zip_reader_create(&reader);
#endif
        struct tm tmu_date;
        
        err = mz_zip_reader_open_file(reader, path.c_str());
        if (err != MZ_OK)
        {
            std::cerr << "ERROR: File not found: " << path << std::endl;
            exit(1);
        }

        err = mz_zip_reader_goto_first_entry(reader);
        if (err != MZ_OK)
        {
            std::cerr << "ERROR: Could not find the first entry in the archive." << std::endl;
            exit(1);
        }

        std::cout << "\nThe archive contains the following files:\n" << std::endl;
        std::cout << "      Date     Time  CRC-32     Name" << std::endl;
        std::cout << "      ----     ----  ------     ----" << std::endl;
        do
        {
            err = mz_zip_reader_entry_get_info(reader, &file_info);
            if (err != MZ_OK)
            {
                std::cerr << "ERROR: Could not get information from entry: " << file_info->filename 
                          << std::endl;
                exit(1);
            }

            mz_zip_time_t_to_tm(file_info->modified_date, &tmu_date);

            // Print entry information.
            printf("      %2.2" PRIu32 "-%2.2" PRIu32 "-%2.2" PRIu32 " %2.2" PRIu32 \
                    ":%2.2" PRIu32 " %8.8" PRIx32 "   %s\n",
                    (uint32_t)tmu_date.tm_mon + 1, (uint32_t)tmu_date.tm_mday,
                    (uint32_t)tmu_date.tm_year % 100,
                    (uint32_t)tmu_date.tm_hour, (uint32_t)tmu_date.tm_min,
                    file_info->crc, file_info->filename);

            err = mz_zip_reader_goto_next_entry(reader);
        } while (err == MZ_OK);
    }

    // Generic error handling.

    else
    {
        std::cerr << "Archive, extract, and/or list functions "
                     "may not be used at the same time." << std::endl;
        exit(1);
    }
}
