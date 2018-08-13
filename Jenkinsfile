@Library("PSL") _

//-----------------------------------------------------------------------------
def getWorkspace()
{
    def root = pwd()
    def index1 = root.lastIndexOf('\\')
    def index2 = root.lastIndexOf('/')
    def count = (index1 > index2 ? index1 : index2) + 1

    // IMPORTANT: This is the master branch to be used as the 
    //            parent branch for any other works. 

    //_CreateCleanDir(root.take(count) + "adsk_contrib/master_build")

    root.take(count) + "adsk_contrib/master_build"
}

// Execute native shell command as per OS
//-----------------------------------------------------------------------------
def runOSCommand(String cmd) 
{
    try {
        if (isUnix()) {
            sh cmd
        } else {
            bat cmd
        }
    } catch(failure) {
        throw failure
    }
}

//-----------------------------------------------------------------------------
def checkoutCode()
{
    ws(getWorkspace()) {
        // checkout the sources from github repo
        // checkout([$class: 'GitSCM', branches: [[name: '5.6.1']], doGenerateSubmoduleConfigurations: false, extensions: 
        // [[$class: 'SubmoduleOption', disableSubmodules: false, parentCredentials: false, recursiveSubmodules: true, 
        // reference: '', timeout: 60, trackingSubmodules: false]], submoduleCfg: [], userRemoteConfigs: [[url: 'git://code.qt.io/qt/qt5.git']]])

        checkout scm

        // Remove all private files first

        runOSCommand("git submodule foreach --recursive \"git clean -dfx\" && git clean -dfx")
        
        runOSCommand("mkdir build_rls")
        runOSCommand("mkdir build_dbg")

        runOSCommand("mkdir install_rls")
        runOSCommand("mkdir install_dbg")

        return pwd()
    }
}

//-----------------------------------------------------------------------------
def downloadPackages(String workDir)
{
    dir(workDir) {
        // download OpenSSL and ICU packages from artifactory 

        artifactDownload = new ors.utils.common_artifactory(steps, env, Artifactory, 'airbuild-svc-user')   

        def downloadspec = """{
            "files": [
                {
                    "pattern": "team-asrd-pilots/openssl/102h/openssl-1.0.2h-win-vc14.zip",
                    "target": "art-bobcat-downloads/"		             
                },
                {
                    "pattern": "team-maya-generic/icu/53.1/icu53_1_vc14.zip",
                    "target": "art-bobcat-downloads/"		             
                }
            ]
        }"""
        
        //artifactDownload.download('https://art-bobcat.autodesk.com/artifactory/', downloadspec)

        // Unpack the downloaded files from Artifactory

        //runOSCommand('7z e art-bobcat-downloads\\openssl\\102h\\openssl-1.0.2h-win-vc14.zip -y -spf -oart-bobcat-downloads')
        //runOSCommand('7z e art-bobcat-downloads\\icu\\53.1\\icu53_1_vc14.zip -y -spf -oart-bobcat-downloads')
    }
}

//-----------------------------------------------------------------------------
def configCompileWin(String workDir)
{
    env.WORKSPACE = workDir

    // Configure and compile OCIO on Windows

    dir(workDir) {
        runOSCommand('ocio_compile.bat')
    }
}

//-----------------------------------------------------------------------------
def configCompileLnx(String workDir)
{
    env.WORKSPACE = workDir

    //  Configure and compile OCIO on Linux

    dir(workDir) {
        dir('build_rls') {
            runOSCommand('''cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install_rls -DOCIO_BUILD_TESTS=ON -DOCIO_BUILD_GPU_TESTS=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_DOCS=OFF -DOCIO_BUILD_APPS=ON -DOCIO_BUILD_PYGLUE=OFF ..''')
            runOSCommand('''make all''')
            runOSCommand('''make test_verbose''')
            runOSCommand('''make install''')
        }
        dir('build_dbg') {
            runOSCommand('''cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install_dbg -DOCIO_BUILD_TESTS=ON -DOCIO_BUILD_GPU_TESTS=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_DOCS=OFF -DOCIO_BUILD_APPS=ON -DOCIO_BUILD_PYGLUE=OFF ..''')
            runOSCommand('''make all''')
            runOSCommand('''make test_verbose''')
            runOSCommand('''make install''')
        }
    }
}

//-----------------------------------------------------------------------------
def configCompileOsx(String workDir)
{
    env.WORKSPACE = workDir

    //  Configure and compile OCIO on OSX

    dir(workDir) {
        dir('build_rls') {
            runOSCommand('''cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../install_rls -DOCIO_BUILD_TESTS=ON -DOCIO_BUILD_GPU_TESTS=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_DOCS=OFF -DOCIO_BUILD_APPS=ON -DOCIO_BUILD_PYGLUE=OFF ..''')
            runOSCommand('''make all''')
            runOSCommand('''make test_verbose''')
            runOSCommand('''make install''')
        }
        dir('build_dbg') {
            runOSCommand('''cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=../install_dbg -DOCIO_BUILD_TESTS=ON -DOCIO_BUILD_GPU_TESTS=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=ON -DOCIO_BUILD_DOCS=OFF -DOCIO_BUILD_APPS=ON -DOCIO_BUILD_PYGLUE=OFF ..''')
            runOSCommand('''make all''')
            runOSCommand('''make test_verbose''')
            runOSCommand('''make install''')
        }
    }
}


// Package creation call for all platforms
//-----------------------------------------------------------------------------	 
def createPackage(String platformTool, String workDir)
{
    env.WORKSPACE = workDir
    env.PLATFORM = platformTool

    dir(workDir) {
		dir('install_rls') {
			if (isUnix()) {
				runOSCommand("""tar -czf OCIO-\$BUILD_NUMBER-${platformTool}-rls.tar.gz *""")
			} else {
				runOSCommand("""7z a -tzip OCIO-%BUILD_NUMBER%-${platformTool}-rls.zip *""")
			}
		}
		dir('install_dbg') {
			if (isUnix()) {
				runOSCommand("""tar -czf OCIO-\$BUILD_NUMBER-${platformTool}-dbg.tar.gz *""")
			} else {
				runOSCommand("""7z a -tzip OCIO-%BUILD_NUMBER%-${platformTool}-dbg.zip *""")
			}
		}
	}
}

// Uploading of packages to artifactory
//-----------------------------------------------------------------------------
def uploadPackagesToArtifactory (String workDir)
{
    dir(workDir) {
        artifactUpload = new ors.utils.common_artifactory(steps, env, Artifactory, 'airbuild-svc-user')   
        
        if (isUnix()) {
            def uploadSpec = """{
                "files": [
                    {
                        "pattern": "install_*/*.tar.gz",
                        "target": "oss-stg-generic/OCIO/$BRANCH_NAME/",
                        "recursive": "false"
                    }
                ]
            }"""

        	artifactUpload.upload('https://art-bobcat.autodesk.com/artifactory/', uploadSpec)
        } else {
            def uploadSpec = """{
                "files": [
                    {
                        "pattern": "install_*/*.zip",
                        "target": "oss-stg-generic/OCIO/$BRANCH_NAME/",
                        "recursive": "false"
                    }
                ]
            }"""

        	artifactUpload.upload('https://art-bobcat.autodesk.com/artifactory/', uploadSpec)
        }
    }
}

// Calling of parallel steps
//-----------------------------------------------------------------------------

def topdir = [:]

//-----------------------------------------------------------------------------
def generateSteps = {lin1, lin2, win, osx ->
    return [ "rhel68" : { node("RHEL68-GPU") { lin1() }},
             "ubuntu16" : { node("OSS-Ubuntu16.04") { lin2() }},
             "Windows" : { node("WIN7X64GPU") { win() }},
             "Mac" : { node("OSS-OSX10.12-Xcode8.3") { osx() }}
            ]
}

stage ('Checkout')
{
    parallel generateSteps(
        {
            topdir["rhel68"] = checkoutCode()
        },
        {
            topdir["ubuntu16"] = checkoutCode()
        },
        {
            topdir["Windows"] = checkoutCode()
        },
        {
            topdir["Mac"] = checkoutCode()
        }
    )
}

stage ('DownloadPackages')
{
    parallel generateSteps(
        {},
        {},
        {
            downloadPackages(topdir["Windows"])
        },
        {}
    )
}

stage ('Config_Compile') 
{
    parallel generateSteps(
        {
            configCompileLnx(topdir["rhel68"])
        },
        {
            configCompileLnx(topdir["ubuntu16"])
        },
        {
            configCompileWin(topdir["Windows"])
        },
        {
            configCompileOsx(topdir["Mac"])
        }
    )
}

stage ('Packaging') 
{
    parallel generateSteps(
        {
            createPackage("rhel68-gcc447", topdir["rhel68"])
        },
        {
            createPackage("ubuntu164-gcc54", topdir["ubuntu16"])
        },
        {
            createPackage("win7-vc14", topdir["Windows"])
        },
        {
            createPackage("osx1012-xcode83", topdir["Mac"])
        }
    )
}

stage ('PublishPackages') 
{
    parallel generateSteps(
        {
            uploadPackagesToArtifactory(topdir["rhel68"])
        },
        {
            uploadPackagesToArtifactory(topdir["ubuntu16"])
        },
        {
            uploadPackagesToArtifactory(topdir["Windows"])
        },
        {
            uploadPackagesToArtifactory(topdir["Mac"])
        }
    )
}
