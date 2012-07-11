//
//  Shader.vsh
//  iosdisplay
//
//  Created by Brian Hall on 7/10/12.
//  Copyright (c) 2012 Sony Pictures Imageworks. All rights reserved.
//

attribute vec4 position;
attribute vec2 texCoord0;

varying lowp vec2 texCoord;

uniform mat4 modelViewProjectionMatrix;

void main()
{    
    gl_Position = modelViewProjectionMatrix * position;
    texCoord = texCoord0;
}
