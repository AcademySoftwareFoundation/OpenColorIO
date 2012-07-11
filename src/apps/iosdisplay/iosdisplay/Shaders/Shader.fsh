//
//  Shader.fsh
//  iosdisplay
//
//  Created by Brian Hall on 7/10/12.
//  Copyright (c) 2012 Sony Pictures Imageworks. All rights reserved.
//

varying lowp vec2 texCoord;

uniform sampler2D imageSampler;
uniform sampler2D lutSampler;

///OCIODisplay///

void main()
{
    gl_FragColor = OCIODisplay(texture2D(imageSampler, texCoord), lutSampler);
}