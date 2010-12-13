
toolbar = nuke.toolbar('Nodes')

if nuke.pluginExists( 'OCIOColorSpace' ):
    toolbar.addCommand( 'Color/OCIOColorSpace',"nuke.createNode('OCIOColorSpace')")

if nuke.pluginExists( 'OCIODisplay' ):
    toolbar.addCommand( 'Color/OCIODisplay',"nuke.createNode('OCIODisplay')")

if nuke.pluginExists( 'OCIOFileTransform' ):
    toolbar.addCommand( 'Color/OCIOFileTransform',"nuke.createNode('OCIOFileTransform')")

if nuke.pluginExists( 'OCIOLogConvert' ):
    toolbar.addCommand( 'Color/OCIOLogConvert',"nuke.createNode('OCIOLogConvert')")
