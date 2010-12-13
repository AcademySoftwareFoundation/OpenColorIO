
import nuke

EBANNER = "OCIO Error: "

try:
    import PyOpenColorIO as OCIO
except Exception, e:
    print '%s%s' % (EBANNER, 'Loading OCIO python module')
    print e

try:
    nuke.load('OCIOColorSpace')
except Exception, e:
    print '%s%s' % (EBANNER, 'Loading OCIOColorSpace')
    print e

try:
    nuke.load('OCIODisplay')
except Exception, e:
    print '%s%s' % (EBANNER, 'Loading OCIODisplay')
    print e

try:
    nuke.load('OCIOFileTransform')
except Exception, e:
    print '%s%s' % (EBANNER, 'loading OCIOFileTransform')
    print e

try:
    nuke.load('OCIOLogConvert')
except Exception, e:
    print '%s%s' % (EBANNER, 'loading OCIOLogConvert')
    print e
