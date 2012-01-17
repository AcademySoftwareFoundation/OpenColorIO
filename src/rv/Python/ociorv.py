import time
import itertools
import contextlib

import rv
from rv.commands import setStringProperty, setIntProperty, setFloatProperty


@contextlib.contextmanager
def timer(msg):
    start = time.time()
    yield
    end = time.time()
    print "%s: %.02fms" % (msg, (end-start)*1000)


def set_lut(proc, nodename):
    """Given an PyOpenColorIO.Processor instance, create a LUT and
    sets it for the specified node
    """

    if proc.isNoOp():
        return

    if hasattr(proc, "hasChannelCrosstalk"):
        # Determine if transform has crosstalk, and switch 1D/3D based on this
        crosstalk = proc.hasChannelCrosstalk()
    else:
        # Assume crosstalk in lieu of latest OCIO Python bindings
        crosstalk = True

    # FIXME: Hardcoded cube sizes
    size_1d = 2048
    size_3d = 32

    if crosstalk:
        _set_lut_3d(proc = proc, nodename = nodename, size = size_3d)
    else:
        _set_lut_1d(proc = proc, nodename = nodename, size = size_1d)

    # TODO: Can also set nodename.lut.inMatrix - could maybe avoid creating a
    # 3D LUT for linear transforms


def _set_lut_3d(proc, nodename, size = 32):
    # FIXME: This clips with >0 scene-linear values, use allocation
    # vars

    # Make noop cube
    size_minus_one = float(size-1)
    one_axis = (x/size_minus_one for x in range(size))
    cube_raw = itertools.product(one_axis, repeat=3)

    # Unpack and fix ordering, by turning [(0, 0, 0), (0, 0, 1), ...]
    # into [0, 0, 0, 1, 0, 0] as generator
    cube_raw = (item for sublist in cube_raw for item in sublist[::-1])

    # Apply transform
    cube_processed = proc.applyRGB(cube_raw)

    # TODO: Use allocation vars to set prelut

    # Set LUT type and size, then LUT values
    setStringProperty("%s.lut.type" % nodename, ["RGB"], False)
    setIntProperty("%s.lut.size" % nodename, [size, size, size], False)
    setFloatProperty("%s.lut.lut" % nodename, cube_processed, True)

    # Activate
    setIntProperty("%s.lut.active" % nodename, [1], False)


def _set_lut_1d(proc, nodename, size = 1024):
    # Make noop ramp
    def gen_noop_ramp(size):
        size_minus_one = float(size-1)
        for x in range(size):
            val = x/size_minus_one
            for i in range(3):
                yield val

    ramp_raw = gen_noop_ramp(size = size)

    # Apply transform
    # TODO: Make applyRGB accept an iterable, rather than requiring a
    # list, to avoid making the intermediate list
    ramp_transformed = proc.applyRGB(ramp_raw)

    # Set LUT type and size, then LUT values
    setStringProperty("%s.lut.type" % nodename, ["RGB"], False)
    setIntProperty("%s.lut.size" % nodename, [size], False)
    setFloatProperty("%s.lut.lut" % nodename, ramp_transformed, True)

    # Activate
    setIntProperty("%s.lut.active" % nodename, [1], False)


def set_noop(nodename):
    """Just ensure sure the LUT is deactivated, in case source changes
    from switch from a non-noop to a noop processor
    """

    is_active = False
    setIntProperty("%s.lut.active" % nodename, [is_active], False)


class PyMyStuffMode(rv.rvtypes.MinorMode):

    def set_display(self, event):
        rv.extra_commands.displayFeedback("Changing display", 2.0)

    def source_setup(self, event):
        args = event.contents().split(";;")
        src = args[0]
        #srctype = args[1]
        srcpath = args[2]

        print "Colour stuff for %s" % srcpath

        filelut_node = rv.extra_commands.associatedNode("RVColor", src)
        looklut_node = rv.extra_commands.associatedNode("RVLookLUT", src)

        # Set 3D LUT, and activate
        import PyOpenColorIO as OCIO
        cfg = OCIO.GetCurrentConfig()

        # FIXME: Need a way to customise this per-facility without
        # modifying this file (try: import ociorv_custom_stuff ?)
        inspace = cfg.parseColorSpaceFromString(srcpath)
        outspace = "srgb8" # nonsense

        testproc = cfg.getProcessor(inspace, outspace)

        if testproc.isNoOp():
            return

        # Get processor from input space to scene-linear, so grading
        # controls etc work as expected
        try:
            input_proc = cfg.getProcessor(inspace, OCIO.Constants.ROLE_SCENE_LINEAR)
        except OCIO.Exception, e:
            print "INFO: Cannot linearise source %s - OCIO error was %s" % (
                srcpath,
                e)
            set_noop(nodename = filelut_node)
            set_noop(nodename = looklut_node)
            return

        # Get processor form scene-linear to output space
        try:
            output_proc = cfg.getProcessor(OCIO.Constants.ROLE_SCENE_LINEAR, outspace)
        except OCIO.Exception, e:
            print "INFO: Cannot apply scene-linear to %s %s - OCIO error was %s" % (
                outspace,
                srcpath,
                e)
            set_noop(nodename = filelut_node)
            set_noop(nodename = looklut_node)
            return

        # LUT to be applied to input file, before any grading etc
        set_lut(proc = input_proc, nodename = filelut_node)

        # LUT after grading etc performed
        set_lut(proc = output_proc, nodename = looklut_node)

        # Update LUT
        rv.commands.updateLUT()


    def __init__(self):
        rv.rvtypes.MinorMode.__init__(self)

        mode_name = "ociorv-mode"

        global_bindings = [
            ("key-down--d", self.set_display, "set display"),
            ("new-source", self.source_setup, "OCIO source setup"),
            ]

        local_bindings = []
        menu = []

        self.init(mode_name,
                  global_bindings,
                  local_bindings,
                  menu)


def createMode():
    return PyMyStuffMode()
