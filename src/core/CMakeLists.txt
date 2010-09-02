
include_directories(
    ${CMAKE_SOURCE_DIR}/export/
)

add_library(OpenColorIO SHARED
    
    CDLTransform.cpp
    ColorSpace.cpp
    ColorSpaceTransform.cpp
    Config.cpp
    DisplayTransform.cpp
    Exception.cpp
    ExponentOps.cpp
    FileFormat3DL.cpp
    FileFormatSpi1D.cpp
    FileFormatSpi3D.cpp
    FileFormatSpiMtx.cpp
    FileTransform.cpp
    GroupTransform.cpp
    GpuAllocationNoOp.cpp
    GpuShaderDesc.cpp
    GpuShaderUtils.cpp
    HashUtils.cpp
    ImageDesc.cpp
    ImagePacking.cpp
    LogOps.cpp
    Lut1DOp.cpp
    Lut3DOp.cpp
    MathUtils.cpp
    MatrixOps.cpp
    Op.cpp
    ParseUtils.cpp
    PathUtils.cpp
    Processor.cpp
    ScanlineHelper.cpp
    Transform.cpp
    
    md5/md5.cpp
    
    pystring/pystring.cpp
    
    tinyxml/tinystr.cpp
    tinyxml/tinyxml.cpp
    tinyxml/tinyxmlerror.cpp
    tinyxml/tinyxmlparser.cpp
)

install(TARGETS OpenColorIO DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${OpenColorIO_SOURCE_DIR}/export/OpenColorIO/OpenColorIO.h
    DESTINATION ${CMAKE_INSTALL_PREFIX}/include/OpenColorIO/)
