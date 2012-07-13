//
//  ViewController.m
//  iosdisplay
//
//  Created by Brian Hall on 7/10/12.
//  Copyright (c) 2012 Sony Pictures Imageworks. All rights reserved.
//

#import "ViewController.h"

#include <OpenColorIO/OpenColorIO.h>
namespace OCIO = OCIO_NAMESPACE;

const int LUT3D_EDGE_SIZE=16;

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// Uniform index.
enum
{
    UNIFORM_MODELVIEWPROJECTION_MATRIX,
    UNIFORM_IMAGE_SAMPLER,
    UNIFORM_LUT_SAMPLER,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

@interface ViewController ()
{
    GLuint _program;
    
    GLKMatrix4 _modelViewProjectionMatrix;

    GLuint _vertexArray;
    GLuint _vertexBuffer;
    GLfloat _vertexData[20];
        
    GLuint _imageTexName;
    GLuint _lutTexName;
    
    GLuint _imageTexWidth;
    GLuint _imageTexHeight;
    GLubyte *_imageTexData;
    
    BOOL _needRefreshTexture;
    BOOL _needRefreshVertexData;
    
    float *_lut3d;
    unsigned char *_lut3dchar;
    
    NSString *_OCIOText;
    
    BOOL _enableOCIO;
    float _exposure;
}
@property (strong, nonatomic) EAGLContext *context;

- (void)setupGL;
- (void)tearDownGL;

- (BOOL)loadOCIO;
- (BOOL)createOCIOTextureAndShader;
- (BOOL)loadShaders;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file;
- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type shaderText:(NSString *)shaderText;
- (BOOL)linkProgram:(GLuint)prog;
- (BOOL)validateProgram:(GLuint)prog;
- (void)loadImage: (NSString *)imageName;
@end

@implementation ViewController

@synthesize context = _context;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    self.preferredFramesPerSecond = 60;
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    _overlayViewController = [[OverlayViewController alloc] initWithNibName:@"OverlayViewController" bundle:nil];
    _overlayViewController.view.frame = CGRectMake(10, 10, view.frame.size.width-20, _overlayViewController.view.frame.size.height);
    _overlayViewController.delegate = self;
    _exposure = _overlayViewController.exposure;
    _enableOCIO = _overlayViewController.enableOCIO;
    [self.view addSubview:_overlayViewController.view];

    _imageTexWidth = 0;
    _imageTexHeight = 0;
    _imageTexData = 0x0;
    
    _needRefreshTexture = TRUE;
    _needRefreshVertexData = TRUE;
    
    [self setupGL];
}

- (void)viewDidUnload
{    
    [super viewDidUnload];
    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
	self.context = nil;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
    return NO;
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];

    const int lutEntries = LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*3;
    _lut3d = new float[lutEntries];
    memset(_lut3d, 0, sizeof(float)*lutEntries);
    
    _lut3dchar = new unsigned char[lutEntries];
    memset(_lut3dchar, 0, sizeof(unsigned char)*lutEntries);
    
    _modelViewProjectionMatrix = GLKMatrix4MakeOrtho(0.0f, 1.0f, 0.0f, 1.0f, 0.01f, 10.0f);
    
    glEnable(GL_TEXTURE);

    glGenTextures(1, &_imageTexName);
    glGenTextures(1, &_lutTexName);
    
    [self loadOCIO];
    [self createOCIOTextureAndShader];

    [self loadShaders];
    
    glGenVertexArraysOES(1, &_vertexArray);
    glBindVertexArrayOES(_vertexArray);
    
    glGenBuffers(1, &_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(_vertexData), _vertexData, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(GLKVertexAttribPosition);
    glVertexAttribPointer(GLKVertexAttribPosition, 3, GL_FLOAT, GL_FALSE, 20, BUFFER_OFFSET(0));
    glEnableVertexAttribArray(GLKVertexAttribTexCoord0);
    glVertexAttribPointer(GLKVertexAttribTexCoord0, 2, GL_FLOAT, GL_FALSE, 20, BUFFER_OFFSET(12));
    
    glBindVertexArrayOES(0);
    
    [self loadImage:@"marci_512_lg8.png"];
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];
    
    glDeleteBuffers(1, &_vertexBuffer);
    glDeleteVertexArraysOES(1, &_vertexArray);
    
    glDeleteTextures(1, &_imageTexName);
    glDeleteTextures(1, &_lutTexName);
    
    if (_lut3d) {
        delete[] _lut3d;
        _lut3d = 0x0;
    }
    
    if (_program) {
        glDeleteProgram(_program);
        _program = 0;
    }
}


#pragma mark - OCIO and image texture setup
- (BOOL)loadOCIO
{
    try
    {
        NSString * configLocation = [[NSBundle mainBundle]
                                     pathForResource:@"config" ofType:@"ocio"
                                     inDirectory:@"OpenColorIOConfigs/spi-vfx"];
        NSLog(@"config bundle: %@", configLocation);
        
        const char * configLocationCstr = [configLocation UTF8String];
        
        OCIO::ConstConfigRcPtr config = OCIO::Config::CreateFromFile(configLocationCstr);
        config->sanityCheck();
        
        OCIO::SetCurrentConfig(config);
    }
    catch(OCIO::Exception & exception)
    {
        NSLog(@"OpenColorIO Error: %s", exception.what());
        OCIO::SetCurrentConfig(OCIO::ConstConfigRcPtr());
    }
    return TRUE;
}

- (BOOL)createOCIOTextureAndShader
{
    std::string fullShaderText;
    try
    {
        OCIO::ConstConfigRcPtr config = OCIO::GetCurrentConfig();
        if (!config) {
            NSLog(@"OpenColorIO current config is null");
            return FALSE;
        }
        
        OCIO::DisplayTransformRcPtr transform = OCIO::DisplayTransform::Create();
        transform->setInputColorSpaceName("lg10"); // marci
        transform->setDisplay(config->getDefaultDisplay());
        transform->setView(config->getDefaultView(config->getDefaultDisplay()));
        
        {
            float exposure = _exposure;
            
            // Add an fstop exposure control (in SCENE_LINEAR)
            float gain = powf(2.0f, exposure);
            const float slope4f[] = { gain, gain, gain, gain };
            float m44[16];
            float offset4[4];
            OCIO::MatrixTransform::Scale(m44, offset4, slope4f);
            OCIO::MatrixTransformRcPtr cc =  OCIO::MatrixTransform::Create();
            cc->setValue(m44, offset4);
            transform->setLinearCC(cc);
        }
        
        OCIO::ConstProcessorRcPtr processor = config->getProcessor(transform);
        
        // Step 1: Create a GPU Shader Description
        OCIO::GpuShaderDesc shaderDesc;
        shaderDesc.setLanguage(OCIO::GPU_LANGUAGE_GLES_2_0);
        shaderDesc.setFunctionName("OCIODisplay");
        shaderDesc.setLut3DEdgeLen(LUT3D_EDGE_SIZE);
        
        _OCIOText = [NSString stringWithCString:processor->getGpuShaderText(shaderDesc)
                                       encoding:NSUTF8StringEncoding];
        
        processor->getGpuLut3D(&_lut3d[0], shaderDesc);   
        
        for (unsigned i=0; i<LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE*3; ++i)
        {
            _lut3dchar[i] = (unsigned char)(MIN(MAX(_lut3d[i], 0.0), 1.0)*255.0);
        }
    }
    catch(OCIO::Exception & exception)
    {
        NSLog(@"OpenColorIO Error: %s", exception.what());
    }
    
    glBindTexture(GL_TEXTURE_2D, _lutTexName);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 LUT3D_EDGE_SIZE, LUT3D_EDGE_SIZE*LUT3D_EDGE_SIZE,
                 0, GL_RGB, GL_UNSIGNED_BYTE, _lut3dchar);
    
    return TRUE;
}


- (void)loadImage: (NSString *)imageName
{
    CGImageRef spriteImage = [UIImage imageNamed:imageName].CGImage;
    if (!spriteImage) {
        NSLog(@"Failed to load image %@", imageName);
    } else {
        size_t width = CGImageGetWidth(spriteImage);
        size_t height = CGImageGetHeight(spriteImage);
        size_t pow2width=1, pow2height=1;
        while (pow2width<width) pow2width <<= 1;
        while (pow2height<height) pow2height <<= 1;
        
        if (_imageTexData==0x0 || pow2width!=_imageTexWidth || pow2height!=_imageTexHeight) {
            
            if (_imageTexData) {
                free(_imageTexData);
            }
            
            _imageTexData = (GLubyte *) calloc(pow2width*pow2height*4, sizeof(GLubyte));
            _imageTexWidth = pow2width;
            _imageTexHeight = pow2height;
        }
        
        // calculate coordinates and uvs based on image size
        GLfloat edgeST[2] = {(GLfloat)width/_imageTexWidth, (GLfloat)height/_imageTexHeight};
        
        GLfloat viewAspect = self.view.frame.size.width / self.view.frame.size.height;
        GLfloat imageAspect = (GLfloat)width/(GLfloat)height;
        GLfloat inset[2] = {0.0f, 0.0f};
        if (imageAspect>viewAspect) {
            GLfloat imageHeight = viewAspect/imageAspect;
            inset[1] = (1.0-imageHeight)/2.0f;
        } else {
            GLfloat imageWidth = imageAspect/viewAspect;
            inset[0] = (1.0-imageWidth)/2.0f;
        }
        
        GLfloat vertexData[20] = 
        {
            //          X,             Y,     Z,                S,              T,
                 inset[0],      inset[1], -1.0f,             0.0f,           1.0f,
                 inset[0], 1.0f-inset[1], -1.0f,             0.0f, 1.0f-edgeST[1],
            1.0f-inset[0], 1.0f-inset[1], -1.0f,        edgeST[0], 1.0f-edgeST[1],
            1.0f-inset[0],      inset[1], -1.0f,        edgeST[0],           1.0f,
        };
        memcpy(_vertexData, vertexData, sizeof(vertexData));
        _needRefreshVertexData = TRUE;
        
        // load image data
        CGContextRef spriteContext = CGBitmapContextCreate(_imageTexData, _imageTexWidth, _imageTexHeight,
                                                           8, _imageTexWidth*4, 
                                                           CGImageGetColorSpace(spriteImage), 
                                                           kCGImageAlphaPremultipliedLast);
        CGContextDrawImage(spriteContext, CGRectMake(0, 0, width, height), spriteImage);
        CGContextRelease(spriteContext);
        
        _needRefreshTexture = TRUE;
    }
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (_needRefreshTexture && _imageTexData && _imageTexWidth && _imageTexHeight) {
        glBindTexture(GL_TEXTURE_2D, _imageTexName);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _imageTexWidth, _imageTexHeight,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, _imageTexData);
        _needRefreshTexture = FALSE;
    }
    

    if (_needRefreshVertexData) {
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(_vertexData), _vertexData, GL_STATIC_DRAW);
        _needRefreshVertexData = FALSE;
    }
    
    // draw
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArrayOES(_vertexArray);
    
    // Render the object again with ES2
    glUseProgram(_program);
    
    glUniformMatrix4fv(uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX], 1, 0, _modelViewProjectionMatrix.m);
    
    glActiveTexture(GL_TEXTURE0); 
    glBindTexture(GL_TEXTURE_2D, _imageTexName);
    glUniform1i(uniforms[UNIFORM_IMAGE_SAMPLER], 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, _lutTexName);
    glUniform1i(uniforms[UNIFORM_LUT_SAMPLER], 1);
    
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

#pragma mark -  OpenGL ES 2 shader compilation

- (BOOL)loadShaders
{
    GLuint vertShader, fragShader;
    NSString *vertShaderPathname, *fragShaderPathname;
    
    if (_program)
    {
        glDeleteProgram(_program);
        _program = 0;
    }
    
    // Create shader program.
    _program = glCreateProgram();
    
    // Create and compile vertex shader.
    vertShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"vsh"];
    if (![self compileShader:&vertShader type:GL_VERTEX_SHADER file:vertShaderPathname]) {
        NSLog(@"Failed to compile vertex shader");
        return NO;
    }
    
    // Create and compile fragment shader.       
    if (_enableOCIO) {
        fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"Shader" ofType:@"fsh"];
    } else {
        fragShaderPathname = [[NSBundle mainBundle] pathForResource:@"NoOCIO" ofType:@"fsh"];
    }
    NSString *fragShaderText = [NSString stringWithContentsOfFile:fragShaderPathname
                                                         encoding:NSUTF8StringEncoding 
                                                            error:nil];
    fragShaderText = [fragShaderText stringByReplacingOccurrencesOfString:@"///OCIODisplay///"
                                                               withString:_OCIOText];
    //NSLog(@"FRAG SHADER TEXT:");
    //NSLog(@"%@", fragShaderText);
    if (![self compileShader:&fragShader type:GL_FRAGMENT_SHADER shaderText:fragShaderText]) {
        NSLog(@"Failed to compile fragment shader");
        return NO;
    }

    
    
    // Attach vertex shader to program.
    glAttachShader(_program, vertShader);
    
    // Attach fragment shader to program.
    glAttachShader(_program, fragShader);
    
    // Bind attribute locations.
    // This needs to be done prior to linking.
    glBindAttribLocation(_program, GLKVertexAttribPosition, "position");
    glBindAttribLocation(_program, GLKVertexAttribTexCoord0, "texCoord0");
    
    // Link program.
    if (![self linkProgram:_program]) {
        NSLog(@"Failed to link program: %d", _program);
        
        if (vertShader) {
            glDeleteShader(vertShader);
            vertShader = 0;
        }
        if (fragShader) {
            glDeleteShader(fragShader);
            fragShader = 0;
        }
        if (_program) {
            glDeleteProgram(_program);
            _program = 0;
        }
        
        return NO;
    }
    
    // Get uniform locations.
    uniforms[UNIFORM_MODELVIEWPROJECTION_MATRIX] = glGetUniformLocation(_program, "modelViewProjectionMatrix");
    uniforms[UNIFORM_IMAGE_SAMPLER] = glGetUniformLocation(_program, "imageSampler");
    uniforms[UNIFORM_LUT_SAMPLER] = glGetUniformLocation(_program, "lutSampler");
    
    // Release vertex and fragment shaders.
    if (vertShader) {
        glDetachShader(_program, vertShader);
        glDeleteShader(vertShader);
    }
    if (fragShader) {
        glDetachShader(_program, fragShader);
        glDeleteShader(fragShader);
    }
    
    return YES;
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type file:(NSString *)file
{
    NSString *shaderText = [NSString stringWithContentsOfFile:file encoding:NSUTF8StringEncoding error:nil];
    return [self compileShader:shader type:type shaderText:shaderText];
}

- (BOOL)compileShader:(GLuint *)shader type:(GLenum)type shaderText:(NSString *)shaderText
{
    GLint status;
    const GLchar *source;
    
    source = (GLchar *)[shaderText UTF8String];
    if (!source) {
        NSLog(@"Failed to load shader");
        return NO;
    }
    
    *shader = glCreateShader(type);
    glShaderSource(*shader, 1, &source, NULL);
    glCompileShader(*shader);
    
#if defined(DEBUG)
    GLint logLength;
    glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetShaderInfoLog(*shader, logLength, &logLength, log);
        NSLog(@"Shader compile log:\n%s", log);
        free(log);
    }
#endif
    
    glGetShaderiv(*shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        glDeleteShader(*shader);
        return NO;
    }
    
    return YES;
}

- (BOOL)linkProgram:(GLuint)prog
{
    GLint status;
    glLinkProgram(prog);
    
#if defined(DEBUG)
    GLint logLength;
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program link log:\n%s", log);
        free(log);
    }
#endif
    
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    
    return YES;
}

- (BOOL)validateProgram:(GLuint)prog
{
    GLint logLength, status;
    
    glValidateProgram(prog);
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0) {
        GLchar *log = (GLchar *)malloc(logLength);
        glGetProgramInfoLog(prog, logLength, &logLength, log);
        NSLog(@"Program validate log:\n%s", log);
        free(log);
    }
    
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);
    if (status == 0) {
        return NO;
    }
    
    return YES;
}

- (void)overlayViewControllerChanged:(OverlayViewController *)overlayViewController
{
    [EAGLContext setCurrentContext:self.context];

    _exposure = overlayViewController.exposure;
    
    if (_enableOCIO != overlayViewController.enableOCIO) {
        _enableOCIO = overlayViewController.enableOCIO;
        [self loadShaders];
    }
    
    [self createOCIOTextureAndShader];
}

@end
