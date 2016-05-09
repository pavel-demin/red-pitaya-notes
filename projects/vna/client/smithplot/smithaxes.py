'''
Library for plotting a fully automatic Smith Chart with various customizable
parameters and well selected default values. It also provides the following
modifications and features:

    - circle shaped drawing area with labels placed around
    - :meth:`plot` accepts complex numbers and numpy.ndarray's
    - lines can be automatically interpolated in evenly spaced steps
    - start/end markers of lines can be modified and rotated
    - gridlines are arcs, which is much more efficient
    - fancy grid option for adaptive grid spacing
    - own tick locators for nice axis labels
    - plot_vswr_circle() for rotating an impedance around the center
    - :meth:`update_scParams` for changing parameters

For making a Smith Chart plot it is sufficient to import :mod:`smithplot` and
create a new subplot with projection set to 'smith'. Parameters can be set
either with keyword arguments or :meth:`update_Params`

Example:

    # creating a new plot and modify parameters afterwards
    import smithplot
    from matplotlib import pyplot as pp
    ax = pp.subplot('111', projection='smith')
    ax.update_scParams(grid_major_color='b')
    ax.cla()
    ## or in short form direct
    #ax = pp.subplot('111', projection='smith', grid_major_color='b')
    pp.plot([1, 1], [0, 1])
    pp.show()

    Note: Supplying parameters to :meth:`subplot` may not always work as
    expected, because subplot uses an index for the axes with a key created
    from all  given parameters. This does not work always, especially if the
    parameters are array-like types (e.g. numpy.ndarray).
'''


from cycler import cycler
from collections import Iterable
from matplotlib.axes import Axes
from matplotlib.cbook import simple_linear_interpolation as linear_interpolation
from matplotlib.lines import Line2D
from matplotlib.markers import MarkerStyle
from matplotlib.patches import Circle, Arc
from matplotlib.path import Path
from matplotlib.spines import Spine
from matplotlib.ticker import Formatter, AutoMinorLocator, Locator
from matplotlib.transforms import Affine2D, BboxTransformTo, Transform
from .smithhelper import EPSILON, TWO_PI, vswr_rotation, lambda_to_rad, ang_to_c, split_complex, convert_args
from types import MethodType, FunctionType
import matplotlib as mp
import numpy as np
from . import smithhelper
from matplotlib.legend_handler import HandlerLine2D
import types


def get_rcParams():
    '''Gets the default values for matploblib parameters'''
    return SmithAxes._rcDefaultParams


def get_scParams():
    '''gets the global default values for all :class:`SmithAxes`'''
    return SmithAxes.scParams


def update_scParams(sc_dict=None, instance=None, **kwargs):
    '''
    Method for updating the standard SmithAxes parameter. If no instance is
    given, the changes are global, but only affect instances created
    afterwards. Parameter can be passed as dictionary or keyword arguments.
    If passed as keyword, the  the seperator '.' must be  replaced with '_'.

    Note: Parameter changes are not always immediate (e.g. changes to the
    grid). It is not recommended to modify parameter after adding anything to
    the plot. For reset call :meth:`cla`.

    Example:
    update_scParams({grid.major: True}) or update_scParams(grid_major=True)

    Valid parameters with default values and description:

        init.updaterc: True
            Updates matplotlib rcParams with SmithAxes defaults. Only affects
            parameter, which have their default value.
            Accepts: boolean
            Note: Can only be set global or as keyword for :meth:`subplot`
            and affects all matplotlib.

        plot.zorder: 5
            Zorder of plotted lines.
            Accepts: integer

        plot.hacklines: True
            Enables the replacement of start and endmarkers.
            Accepts: boolean
            Note: Uses ugly code injection and may causes unexpected behavior.

        plot.rotatemarker: True
            Rotates the endmarker in the direction of its line.
            Accepts: boolean
            Note: needs plot.hacklines=True

        plot.startmarker: 's',
            Marker for the first point of a line, if it has more than 1 point.
            Accepts: None or see matplotlib.markers.MarkerStyle()
            Note: needs plot.hacklines=True

        plot.marker: 'o'
            Marker used for linepoints.
            Accepts: None or see matplotlib.markers.MarkerStyle()

        plot.endmarker: '^',
            Marker for the last point of a line, if it has more than 1 point.
            Accepts: None or see matplotlib.markers.MarkerStyle()
            Note: needs plot.hacklines=True

        grid.zorder : 1
            Zorder of the gridlines.
            Accepts: integer
            Note: may not work as expected

        grid.locator.precision: 2
            Sets the number of significant decimals per decade for the
            Real and Imag MaxNLocators. Example with precision 2:
                1.12 -> 1.1, 22.5 -> 22, 135 -> 130, ...
            Accepts: integer
            Note: value is an orientation, several exceptions are implemented

        grid.major.enable: True
            Enables the major grid.
            Accepts: boolean

        grid.major.linestyle: 'solid'
            Major gridline style.
            Accepts: see matplotlib.patches.Patch.set_linestyle()

        grid.major.linewidth: 1
            Major gridline width.
            Accepts: float

        grid.major.color: '0.2'
            Major gridline color.
            Accepts: matplotlib color

        grid.major.xmaxn: 10
            Maximum number of spacing steps for the real axis.
            Accepts: integer

        grid.major.ymaxn: 16
            Maximum number of spacing steps for the imaginary axis.
            Accepts: integer

        grid.major.fancy: True
            Draws a fancy major grid instead of the standard one.
            Accepts: boolean

        grid.major.fancy.threshold: (100, 50)
            Minimum distance times 1000 between two gridlines relative to
            total plot size 2. Either tuple for individual real and
            imaginary distances or single value for both.
            Accepts: (float, float) or float

        grid.minor.enable: True
            Enables the minor grid.
            Accepts: boolean

        grid.minor.linestyle: (0, (0.2, 2))
            Minor gridline style.
            Accepts: see matplotlib.patches.Patch.set_linestyle()

        grid.minor.linewidth: 0.75
            Minor gridline width.
            Accepts: float

        grid.minor.color: 0.4
            Minor gridline color.
            Accepts: matplotlib color

        grid.minor.xauto: 4
            Maximum number of spacing steps for the real axis.
            Accepts: integer

        grid.minor.yauto: 4
            Maximum number of spacing steps for the imaginary axis.
            Accepts: integer

        grid.minor.fancy: True
            Draws a fancy minor grid instead the standard one.
            Accepts: boolean

        grid.minor.fancy.dividers: [1, 2, 3, 5, 10, 20]
            Divisions for the fancy minor grid, which are selected by
            comparing the distance of gridlines with the threshold value.
            Accepts: list of integers

        grid.minor.fancy.threshold: 25
            Minimum distance for using the next bigger divider. Value times
            1000 relative to total plot size 2.
            Accepts: float

        Note: gridlines are matplotlib.patches.Patch instances, which are no
        Line2D objects. Therefore Line2D parameter can not be used.

        path.default_interpolation: 75
            Default number of interpolated steps between two points of a
            line, if interpolation is used.
            Accepts: integer

        axes.xlabel.rotation: 90
           Rotation of the real axis labels in degree.
           Accepts: float

        axes.xlabel.fancybox: {"boxstyle": "round4,pad=0.4"
                                 "facecolor": 'w'
                                 "edgecolor": "w"
                                 "mutation_aspect": 0.75}
            FancyBboxPatch parameter for the labels bounding box.
            Accepts: dictionary with rectprops

        axes.ylabel.correction: (-2, 0)
            Correction for the labels of the imaginary axis. Usually needs to
            be adapted when fontsize changes 'font.size'.
            Accepts: (float, float)

        axes.radius: 0.43
            Radius of the plotting area. Usually needs to be adapted to
            the size of the figure.
            Accepts: float

        axes.scale: 1
            Defines internal normalisation and is used for scaling the axis.
            Does not rescale supplied data.
            Accepts: float

        axes.norm: None
            If not None, a textbox with 'Norm: %d Ohm' is put in the lower
            left corner.
            Accepts: None or float

        symbol.infinity: u"\u221E"
            Symbol for infinity (can be normal text as well).
            Accepts: string

        symbol.infinity.correction: 7
            Correction of size for the infinity symbol, because normal symbol
            seems smaller than other letters.
            Accepts: float

    Note: The keywords are processed after the dictionary and override
    possible double entries.
    '''
    if not sc_dict:
        sc_dict = {}
    assert isinstance(sc_dict, dict)

    key_error = lambda k: KeyError("key '%s' is not in scParams" % k)
    if instance is None:
        scParams = SmithAxes.scDefaultParams
    else:
        scParams = instance.scParams

    for key, value in sc_dict.items():
        if key in scParams:
            scParams[key] = value
        else:
            raise key_error(key)

    for key, value in kwargs.items():
        key_dot = key.replace("_", ".")
        if key_dot in scParams:
            scParams[key_dot] = value
        else:
            raise key_error(key)


class SmithAxes(Axes):
    '''
    The :class:`SmithAxes` provides a :class:`matplotlib.axes.Axes` for
    drawing a full automatic Smith Chart it also provides own derivatives of

        - :class:`matplotlib.transforms.Transform`
            -> :class:`MoebiusTransform`
            -> :class:`InvertedMoebiusTransform`
            -> :class:`PolarTranslate`
        - :class:`matplotlib.ticker.Locator`
            -> :class:`RealMaxNLocator`
            -> :class:`ImagMaxNLocator`
            -> :class:`SmithAutoMinorLocator`
        - :class:`matplotlib.ticker.Formatter`
            -> :class:`RealFormatter`
            -> :class:`ImagFormatter`
    '''

    name = 'smith'

    # constants used for indicating values near infinity, which are all
    # transformed into one point
    _inf = smithhelper.INF
    _near_inf = 0.9 * smithhelper.INF
    _ax_lim_x = _near_inf
    _ax_lim_y = _inf

    # default parameter for matplotlib
    _rcDefaultParams = {"font.size": 15,
                   "legend.fontsize": 16,
                   "lines.linestyle": "-",
                   "lines.linewidth": 2,
                   "lines.markersize": 8,
                   "lines.markeredgewidth": 1,
                   "axes.prop_cycle": cycler('color',
                                      ["#FF4848", # red
                                       "#31B404", # blue
                                       "#0276FD", # green
                                       "#FFB428", # orange
                                       "#01C5BB", # teal
                                       "#CD69C9", # pink
                                       "0.65", # dark grey
                                       "0.45"]), # light grey
                   "xtick.labelsize": 14,
                   "xtick.major.pad": 0,
                   "ytick.labelsize": 15,
                   "ytick.major.pad": 10,
                   "legend.fancybox": False,
                   "legend.shadow": True,
                   "legend.markerscale": 0.75,
                   "legend.numpoints": 3,
                   "axes.axisbelow": True}

    scDefaultParams = {"init.updaterc": True,
                "plot.zorder": 5,
                "plot.hacklines": True,
                "plot.rotatemarker": True,
                "plot.startmarker": "s",
                "plot.marker": "o",
                "plot.endmarker": "^",
                   "grid.zorder" : 1,
                   "grid.locator.precision": 2,
                   "grid.major.enable": True,
                   "grid.major.linestyle": '-',
                   "grid.major.linewidth": 1,
                   "grid.major.color": "0.2",
                   "grid.major.xmaxn": 10,
                   "grid.major.ymaxn": 16,
                   "grid.major.fancy": True,
                   "grid.major.fancy.threshold": (100, 50),
                   "grid.minor.enable": True,
                   "grid.minor.linestyle": ":",
                   "grid.minor.capstyle": str("round"),
                   "grid.minor.dashes": [0.2, 2],
                   "grid.minor.linewidth": 0.75,
                   "grid.minor.color": "0.4",
                   "grid.minor.xauto": 4,
                   "grid.minor.yauto": 4,
                   "grid.minor.fancy": True,
                   "grid.minor.fancy.dividers": [1, 2, 3, 5, 10, 20],
                   "grid.minor.fancy.threshold": 25,
                   "path.default_interpolation": 75,
                   "axes.xlabel.rotation": 90,
                   "axes.xlabel.fancybox": {"boxstyle": "round4,pad=0.4",
                                            "facecolor": 'w',
                                            "edgecolor": "w",
                                            "mutation_aspect": 0.75},
                   "axes.ylabel.correction": (-2, 0),
                   "axes.radius": 0.43,
                   "axes.scale": 1,
                   "axes.norm": None,
                   "symbol.infinity": "\u221E",
                   "symbol.infinity.correction": 7}

    def __init__(self, *args, **kwargs):
        '''
        Builds a new :class:`SmithAxes` instance. If 'init.updaterc' is set
        updates the global rcParams of matplotlib. For futher details see:

            :meth:`update_scParams`
            :class:`matplotlib.axes.Axes`
        '''
        self.scParams = self.scDefaultParams.copy()

        # get parameter for Axes and remove from kwargs
        axes_kwargs = {}
        for key in kwargs.copy():
            key_dot = key.replace("_", ".")
            if not (key_dot in self.scParams or \
                    key_dot in self._rcDefaultParams):
                axes_kwargs[key] = kwargs.pop(key_dot)

        self.update_scParams(**kwargs)

        # if 'init.updaterc' is True, all matplotlib rc parameter
        # which are unmodified are updated to the Smith defaults
        if self._get_key("init.updaterc"):
            for key, value in self._rcDefaultParams.items():
                if mp.rcParams[key] == mp.rcParamsDefault[key]:
                    mp.rcParams[key] = value

        Axes.__init__(self, *args, **axes_kwargs)
        self.set_aspect(1, adjustable='box', anchor='C')

    def _get_key(self, key):
        '''
        Get a key from the local parameter dictionary or from global
        matplotlib rcParams.

        Keyword arguments:

            *key*:
                Key to get from scParams or matplotlib.rcParams
                Accepts: string

        Returns:

            *value*:
                Value got from scParams or rcParams with key
        '''
        if key in self.scParams:
            return self.scParams[key]
        elif key in mp.rcParams:
            return mp.rcParams[key]
        else:
            raise KeyError("%s is not a valid key" % key)

    def update_scParams(self, sc_dict=None, **kwargs):
        '''Updates the local parameter of this instance.
        For more details see :func:`update_scParams`'''
        if not sc_dict:
            sc_dict = {}
        update_scParams(sc_dict=sc_dict, instance=self, **kwargs)

    def _init_axis(self):
        self.xaxis = mp.axis.XAxis(self)
        self.yaxis = mp.axis.YAxis(self)
        self._update_transScale()

    def cla(self):
        # Don't forget to call the base class
        Axes.cla(self)

        self._fancy_majorarcs = []
        self._fancy_minorarcs = []
        self._normbox = None
        self._scale = self._get_key("axes.scale")
        self._current_zorder = self._get_key("plot.zorder")

        self.xaxis.set_major_locator(self.RealMaxNLocator(self, self._get_key("grid.major.xmaxn")))
        self.yaxis.set_major_locator(self.ImagMaxNLocator(self, self._get_key("grid.major.ymaxn")))

        self.xaxis.set_minor_locator(self.SmithAutoMinorLocator(self._get_key("grid.minor.xauto")))
        self.yaxis.set_minor_locator(self.SmithAutoMinorLocator(self._get_key("grid.minor.yauto")))

        self.xaxis.set_ticks_position('none')
        self.yaxis.set_ticks_position('none')

        Axes.set_xlim(self, 0, self._ax_lim_x)
        Axes.set_ylim(self, -self._ax_lim_y, self._ax_lim_y)

        for label in self.get_xticklabels():
            label.set_verticalalignment('center')
            label.set_horizontalalignment('center')
            label.set_rotation(self._get_key("axes.xlabel.rotation"))
            label.set_bbox(self._get_key("axes.xlabel.fancybox"))
            self.add_artist(label)  # if not readded, labels are drawn behind grid

        for tick, loc in zip(self.yaxis.get_major_ticks(),
                             self.yaxis.get_majorticklocs()):
            # workaround for fixing to small infinity symbol
            if abs(loc) > self._near_inf:
                tick.label.set_size(tick.label.get_size() +
                                    self._get_key("symbol.infinity.correction"))

            tick.label.set_verticalalignment('center')

            x = np.real(self._moebius_z(loc * 1j))
            if x < -0.1:
                tick.label.set_horizontalalignment('right')
            elif x > 0.1:
                tick.label.set_horizontalalignment('left')
            else:
                tick.label.set_horizontalalignment('center')

        self.yaxis.set_major_formatter(self.ImagFormatter(self))
        self.xaxis.set_major_formatter(self.RealFormatter(self))

        norm = self._get_key("axes.norm")
        if norm is not None:
            x, y = split_complex(self._moebius_inv_z(-1 - 1j))
            self._normbox = self.text(x, y, "Norm: %d\u2126" % norm)
            self._normbox.set_verticalalignment("center")

            px = self._get_key("ytick.major.pad")
            py = px + 0.5 * self._normbox.get_size()
            self._normbox.set_transform(self._yaxis_correction + \
                                        Affine2D().translate(-px, -py))

        for grid in ['minor', "major"]:
            self.grid(b=self._get_key("grid.%s.enable" % grid), which=grid)

    def _set_lim_and_transforms(self):
        r = self._get_key("axes.radius")
        self.transProjection = self.MoebiusTransform(self)
        self.transAffine = Affine2D().scale(r, r) \
                                     .translate(0.5, 0.5)
        self.transAxes = BboxTransformTo(self.bbox)
        self.transMoebius = self.transAffine + \
                            self.transAxes
        self.transData = self.transProjection + \
                         self.transMoebius

        self._xaxis_pretransform = Affine2D().scale(1, 2 * self._ax_lim_y) \
                                             .translate(0, -self._ax_lim_y)
        self._xaxis_transform = self._xaxis_pretransform + \
                                self.transData
        self._xaxis_text1_transform = Affine2D().scale(1.0, 0.0) + \
                                      self.transData

        self._yaxis_stretch = Affine2D().scale(self._ax_lim_x, 1.0)
        self._yaxis_correction = self.transData + \
                                 Affine2D().translate(*self._get_key("axes.ylabel.correction"))
        self._yaxis_transform = self._yaxis_stretch + \
                                self.transData
        self._yaxis_text1_transform = self._yaxis_stretch + \
                                      self._yaxis_correction

    def get_xaxis_transform(self, which='grid'):
        assert which in ['tick1', 'tick2', 'grid']
        return self._xaxis_transform

    def get_xaxis_text1_transform(self, pixelPad):
        return self._xaxis_text1_transform, 'center', 'center'

    def get_yaxis_transform(self, which='grid'):
        assert which in ['tick1', 'tick2', 'grid']
        return self._yaxis_transform

    def get_yaxis_text1_transform(self, pixelPad):
        if hasattr(self, 'yaxis') and len(self.yaxis.majorTicks) > 0:
            font_size = self.yaxis.majorTicks[0].label.get_size()
        else:
            font_size = self._get_key("font.size")
        return self._yaxis_text1_transform + self.PolarTranslate(self, pad=pixelPad, font_size=font_size), 'center', 'center'

    def _gen_axes_patch(self):
        return Circle((0.5, 0.5), self._get_key("axes.radius") + 0.015)

    #TODO: RPC: I added arguments here to match with the matploblib Axes base class. Need to make sure this works.
    def _gen_axes_spines(self, locations=None, offset=0.0, units='inches'):
        return {SmithAxes.name: Spine.circular_spine(self, (0.5, 0.5), self._get_key("axes.radius"))}

    def set_xscale(self, *args, **kwargs):
        if args[0] != 'linear':
            raise NotImplementedError
        Axes.set_xscale(self, *args, **kwargs)

    def set_yscale(self, *args, **kwargs):
        if args[0] != 'linear':
            raise NotImplementedError
        Axes.set_yscale(self, *args, **kwargs)

    def set_xlim(self, *args, **kwargs):
        '''xlim is immutable and always set to (0, infinity)'''
        Axes.set_xlim(self, 0, self._ax_lim_x)

    def set_ylim(self, *args, **kwargs):
        '''ylim is immutable and always set to (-infinity, infinity)'''
        Axes.set_ylim(self, -self._ax_lim_y, self._ax_lim_y)

    def format_coord(self, re, im):
        sgn = "+" if im > 0 else "-"
        return "%.5f %s %.5fj" % (re, sgn, abs(im)) if re > 0 else ""

    def get_data_ratio(self):
        return 1.0

    def can_zoom(self):
        return False

    #TODO: Does this need to do something?
    def start_pan(self, x, y, button):
        pass
#         x, _ = self.transData.inverted().transform([[x, y]])[0]
#         self._scale = x
#         for a in self.get_children():
#             if hasattr(a, "_transformed_path"):
#                 a._transformed_path.invalidate()

    #TODO: Do either of these need to do something?
    def end_pan(self):
        pass

    def drag_pan(self, button, key, x, y):
        pass

    def _moebius_z(self, *args):
        '''
        Basic transformation.

        Arguments:

            *z*:
                Complex number or numpy.ndarray with dtype=complex

            *x, y*:
                Float numbers or numpy.ndarray's with dtype not complex

        Returns:

            *w*:
                Performs w = (z - k) / (z + k) with k = 'axes.scale'
                Type: Complex number or numpy.ndarray with dtype=complex
        '''
        return smithhelper.moebius_z(convert_args(*args), self._scale)

    def _moebius_inv_z(self, *args):
        '''
        Basic inverse transformation.

        Arguments:

            *z*:
                Complex number or numpy.ndarray with dtype=complex

            *x, y*:
                Float numbers or numpy.ndarray's with dtype not complex

        Returns:

            *w*:
                Performs w = k * (1 - z) / (1 + z) with k = 'axes.scale'
                Type: Complex number or numpy.ndarray with dtype=complex
        '''
        return smithhelper.moebius_inv_z(convert_args(*args), self._scale)

    def real_interp1d(self, x, steps):
        '''
        Interpolates the given vector as real numbers in the way, that they
        are evenly spaced after a transformation with imaginary part 0.

        Keyword Arguments

            *x*:
                Real values to interpolate.
                Accepts: 1D iterable (e.g. list or numpy.ndarray)

            *steps*:
                Number of steps between two points.
                Accepts: integer
        '''
        return self._moebius_inv_z(linear_interpolation(self._moebius_z(np.array(x)), steps))

    def imag_interp1d(self, y, steps):
        '''
        Interpolates the given vector as imaginary numbers in the way, that
        they are evenly spaced after a transformation with real part 0.

        Keyword Arguments

            *y*:
                Imaginary values to interpolate.
                Accepts: 1D iterable (e.g. list or numpy.ndarray)

            *steps*:
                Number of steps between two points.
                Accepts: integer
        '''
        angs = np.angle(self._moebius_z(np.array(y) * 1j)) % TWO_PI
        i_angs = linear_interpolation(angs, steps)
        return np.imag(self._moebius_inv_z(ang_to_c(i_angs)))

    def legend(self, *args, **kwargs):
        this_axes = self
        class SmithHandlerLine2D(HandlerLine2D):
            def create_artists(self, legend, orig_handle,
                xdescent, ydescent, width, height, fontsize,
                trans):
                legline, legline_marker = HandlerLine2D.create_artists(self, legend, orig_handle, xdescent, ydescent, width, height, fontsize, trans)

                if hasattr(orig_handle, "_markerhacked"):
                    this_axes._hack_linedraw(legline_marker, True)
                return [legline, legline_marker]
        return Axes.legend(self, *args, handler_map={Line2D : SmithHandlerLine2D()}, **kwargs)

    def plot(self, *args, **kwargs):
        '''
        Plot the given data into the Smith Chart. Behavior similar to basic
        :meth:`matplotlib.axes.Axes.plot`, but with some extensions:

            - Additional support for complex data. Complex values must be
            either of type 'complex' or a numpy.ndarray with dtype=complex.
            - If 'zorder' is not provided, the current default value is used.
            - If 'marker' is not providet, the default value is used.
            - Extra keywords are added.

        Extra keyword arguments:

            *no_transform*:
                If set, given data points are plotted directly, without
                transforming them into smith space.

            *path_interpolation*:
                If set, interpolates the path with the given steps.  If the
                value is 0, a bezier arc is drawn.

            *markerhack*:
                If set, activates the manipulation of start and end markern
                of the created line.

            *rotate_marker*:
                if *markerhack* is active, rotates the endmarker in direction
                of the corresponding path.

        See :meth:`matplotlib.axes.Axes.plot` for mor details
        '''
        new_args = ()
        for arg in args:
            if isinstance(arg, np.ndarray) and arg.dtype == np.complex64 or \
               isinstance(arg, np.ndarray) and arg.dtype == np.float32 or \
               isinstance(arg, complex) or isinstance(arg, float):
                new_args += (np.real(arg), np.imag(arg))
            else:
                new_args += (arg,)

        if 'zorder' not in kwargs:
            kwargs['zorder'] = self._current_zorder
            self._current_zorder += 0.001
        if 'marker' not in kwargs:
            kwargs['marker'] = self._get_key("plot.marker")

        if "path_interpolation" in kwargs:
            steps = kwargs.pop("path_interpolation")
        else:
            steps = None

        if "markerhack" in kwargs:
            markerhack = kwargs.pop("markerhack")
        else:
            markerhack = self._get_key("plot.hacklines")

        if "rotate_marker" in kwargs:
            rotate_marker = kwargs.pop("rotate_marker")
        else:
            rotate_marker = self._get_key("plot.rotatemarker")

        if "no_transform" in kwargs:
            no_transform = kwargs.pop("no_transform")
        else:
            no_transform = False

        lines = Axes.plot(self, *new_args, **kwargs)
        for line in lines:
            if no_transform:
                x, y = line.get_data()
                z = self._moebius_inv_z(x + y * 1j)
                line.set_data(z.real, z.imag)

            if steps is not None:
                line.get_path()._interpolation_steps = steps

            if markerhack:
                self._hack_linedraw(line, rotate_marker)

        return lines

    #TODO: Does impedance need to do something?
    def plot_vswr_circle(self,
                         point,
                         impedance=None,
                         real=None,
                         imag=None,
                         lambda_rotation=None,
                         solution2=False,
                         direction="counterclockwise",
                         **kwargs):
        '''
        Plot an arc from point p=(x, y) around (1, 0) with given destination
        and orientation.

        Keyword arguments:

        *real*, *imag*, *lambda_rotation*, *solution2*, *direction*:
            see documentation of :meth:`smithhelper.vswr_rotation`

        *args*:
            startpoint, specified either with one complex number z or two
            floats x, y standing for real and imaginary part

        **kwargs*:
            keyword arguments passed to to :meth:`plot` command

        Returns: Line2D object created

        Example: plot_vswr_circle(1 + 1j, real=1, direction='ccw')
        '''
        assert isinstance(point, complex) or len(point) == 2
        if isinstance(point, complex):
            x, y = split_complex(point)
        else:
            x, y = point

        # interpolate the line with default value
        steps = self._get_key("path.default_interpolation")

        # workaround for full circle, prevents plotting the endmarker
        if real is None and imag is None and lambda_rotation is None:
            kwargs["markevery"] = steps + 1
        else:
            kwargs["markevery"] = steps

        z0, _z1, lmb = vswr_rotation(x, y, self._scale, real, imag, lambda_rotation, solution2, direction)
        ang = lambda_to_rad(lmb)

        z = self._moebius_inv_z(self._moebius_z(z0) * ang_to_c(np.linspace(0, ang, steps + 1)))
        return self.plot(z, **kwargs)[0]

    def grid(self,
             b=None,
             which='major',
             axis='both',
             fancy=None,
             dividers=None,
             threshold=None,
             **kwargs):
        '''
        Complete rewritten grid function. Gridlines are replaced with Arcs,
        which reduces the amount of points to store and increases speed. The
        grid consist of a minor and major part, which can be drawn either as
        standard with lines from axis to axis, or fancy with dynamic spacing
        and length adaption.

        Keyword arguments:

            *b*:
                Enables or disables the selected grid.
                Accepts: boolean

            *which*:
                The grid to be drawn.
                Accepts: ['major', 'minor', 'both']

            *axis*:
                The axis to be drawn. x=real and y=imaginary
                Accepts: ['x', 'y', 'both']
                Note: if fancy is set, only 'both' is valid

            *fancy*:
                If set to 'True', draws the grid on the fancy way.
                Accepts: boolean

            *dividers*:
                Adaptive divisions for the minor fancy grid.
                Accepts: array with integers
                Note: has no effect on major and non-fancy grid

            *threshold*:
                Threshold for dynamic adaption of spacing and line length. Can
                be specified for both axis together or each seperatly.
                Accepts: float or (float, float)

            **kwargs*:
                Keyword arguments passed to the gridline creator.
                Note: Gridlines are :class:`matplotlib.patches.Patch` and does
                not accept all arguments :class:`matplotlib.lines.Line2D`
                accepts.
        '''
        assert which in ["both", "major", "minor"]
        assert axis in ["both", "x", "y"]
        assert fancy in [None, True, False]

        def update_param(grid):
            kw = kwargs.copy()
            if 'zorder' not in kw:
                kw['zorder'] = self._get_key("grid.zorder")

            for key in ["linestyle", "linewidth", "color"]:
                if key not in kw:
                    kw[key] = self._get_key("grid.%s.%s" % (grid, key))

            return kw

        def draw_standard(xticks, yticks, param, arc_storage):
            if axis in ["both", "x"]:
                for xs in xticks:
                    arc_storage.append(self.add_realarc(xs, -self._inf, self._inf, **param))

            if axis in ["both", "y"]:
                for ys in yticks:
                    arc_storage.append(self.add_imagarc(ys, 0, self._inf, **param))

        def check_fancy(yticks):
            # checks if the imaginary axis is symetric
            len_y = (len(yticks) - 1) // 2
            if not (len(yticks) % 2 == 1 and \
                    (yticks[len_y:] + yticks[len_y::-1] < EPSILON).all()):
                raise ValueError("fancy minor grid is only supported for zero-symetric imaginary grid - e.g. ImagMaxNLocator")
            return  yticks[len_y:]

        def split_threshold(threshold):
            if isinstance(threshold, tuple):
                thr_x, thr_y = threshold
            else:
                thr_x = thr_y = threshold
            assert thr_x > 0 and thr_y > 0

            return (thr_x / 1000, thr_y / 1000)

        if fancy is None:
            fancy_major = self._get_key("grid.major.fancy")
            fancy_minor = self._get_key("grid.minor.fancy")
        else:
            fancy_major = fancy_minor = fancy

        if (fancy_major or fancy_minor) and axis != 'both':
            raise NotImplementedError("fancy grid is only supported for both axis")

        if which in ['both', 'minor']:
            # remove the old grid
            try:
                for arc in self._fancy_minorarcs:
                    arc.remove()
            except:
                pass
            self._fancy_minorarcs = []

            if b:
                param = update_param("minor")
                if "dash_capstyle" not in param:
                    param["dash_capstyle"] = self._get_key("grid.minor.capstyle")
                if "dashes" not in param:
                    param["dashes"] = self._get_key("grid.minor.dashes")

                if fancy_minor:
                    # 1. Step: get x/y grid data
                    xticks = np.sort(self.xaxis.get_majorticklocs())
                    yticks = np.sort(self.yaxis.get_majorticklocs())
                    assert len(xticks) > 0 and len(yticks) > 0

                    yticks = check_fancy(yticks)

                    if dividers is None:
                        dividers = self._get_key("grid.minor.fancy.dividers")
                    assert len(dividers) > 0
                    dividers = np.sort(dividers)

                    if threshold is None:
                        threshold = self._get_key("grid.minor.fancy.threshold")

                    thr_x, thr_y = split_threshold(threshold)
                    len_x, len_y = len(xticks) - 1, len(yticks) - 1

                    # 2. Step: calculate optimal gridspacing for each quadrant
                    d_mat = np.ones((len_x, len_y, 2))

                    for i in range(len_x):
                        for k in range(len_y):
                            x0, x1 = xticks[i:i + 2]
                            y0, y1 = yticks[k:k + 2]

                            xm = self.real_interp1d([x0, x1], 2)[1]
                            ym = self.imag_interp1d([y0, y1], 2)[1]

                            x_div = y_div = dividers[0]

                            for div in dividers[1:]:
                                if abs(self._moebius_z(x1 - (x1 - x0) / div, ym) - self._moebius_z(x1, ym)) > thr_x:
                                    x_div = div
                                else:
                                    break

                            for div in dividers[1:]:
                                if abs(self._moebius_z(xm, y1) - self._moebius_z(xm, y1 - (y1 - y0) / div)) > thr_y:
                                    y_div = div
                                else:
                                    break

                            d_mat[i, k] = [x_div, y_div]

                    # 3. Steps: optimize spacing
                    # ensure the x-spacing declines towards infinity
                    d_mat[:-1, 0, 0] = list(map(np.max, list(zip(d_mat[:-1, 0, 0],
                                                       d_mat[1:, 0, 0]))))

                    # find the values which are near (0, 0.5) on the plot
                    idx = np.searchsorted(xticks,
                                          self._moebius_inv_z(0)) + 1
                    idy = np.searchsorted(yticks,
                                          np.imag(self._moebius_inv_z(1j)))

                    # extend the values around the center towards the border
                    if idx > idy:
                        for d in range(idy):
                            delta = idx - idy + d
                            d_mat[delta, :d + 1] = d_mat[:delta, d] = d_mat[delta, 0]
                    else:
                        for d in range(idx):
                            delta = idy - idx + d
                            d_mat[:d + 1, delta] = d_mat[d, :delta] = d_mat[d, 0]

                    # 4. Step: gather and optimize the lines
                    x_lines, y_lines = ([], [])

                    for i in range(len_x):
                        x0, x1 = xticks[i:i + 2]

                        for k in range(len_y):
                                y0, y1 = yticks[k:k + 2]

                                x_div, y_div = d_mat[i, k]

                                for xs in np.linspace(x0, x1, x_div + 1)[1:]:
                                    x_lines.append([xs, y0, y1])
                                    x_lines.append([xs, -y1, -y0])


                                for ys in np.linspace(y0, y1, y_div + 1)[1:]:
                                    y_lines.append([ys, x0, x1])
                                    y_lines.append([-ys, x0, x1])

                    # round values to prevent float inaccuarcy
                    x_lines = np.round(np.array(x_lines), 7)
                    y_lines = np.round(np.array(y_lines), 7)

                    def merge_lines(lines, ds):
                        # elimenate lines where the endpoint is startpoint of
                        # another line and vice versa.
                        data = lines[lines[:, 0] == ds, 1:]
                        data = data[data[:, 0].argsort()]
                        data[:, 1] = np.roll(data[:, 1], 1)
                        data = data[data[:, 0] != data[:, 1]]
                        data[:, 0] = np.roll(data[:, 0], 1)
                        return data

                    for xs in np.unique(x_lines[:, 0]):
                        for y0, y1 in merge_lines(x_lines, xs):
                            self._fancy_minorarcs.append(self.add_realarc(xs, y0, y1, **param))

                    for ys in np.unique(y_lines[:, 0]):
                        for x0, x1 in merge_lines(y_lines, ys):
                            self._fancy_minorarcs.append(self.add_imagarc(ys, x0, x1, **param))

                else:
                    xticks = np.sort(self.xaxis.get_minorticklocs())
                    yticks = np.sort(self.yaxis.get_minorticklocs())

                    draw_standard(xticks, yticks, param, self._fancy_minorarcs)

        if which in ['both', 'major']:
            try:
                for arc in self._fancy_majorarcs:
                    arc.remove()
            except:
                pass

            self._fancy_majorarcs = []

            if b:
                param = update_param('major')
                xticks = np.sort(self.xaxis.get_majorticklocs())
                yticks = np.sort(self.yaxis.get_majorticklocs())

                if fancy_major:
                    assert len(xticks) > 0 and len(yticks) > 0
                    yticks = check_fancy(yticks)

                    if threshold is None:
                        threshold = self._get_key("grid.major.fancy.threshold")

                    thr_x, thr_y = split_threshold(threshold)

                    # draw the 0 line
                    self._fancy_majorarcs.append(self.add_imagarc(yticks[0], 0, xticks[-1], **param))

                    tmp_yticks = yticks.copy()
                    for xs in xticks[1:]:
                        k = 1
                        while k < len(tmp_yticks):
                            y0, y1 = tmp_yticks[k - 1:k + 1]
                            if abs(self._moebius_z(xs, y0) - self._moebius_z(xs, y1)) < thr_x:
                                self._fancy_majorarcs.append(self.add_imagarc(y1, 0, xs, **param))
                                self._fancy_majorarcs.append(self.add_imagarc(-y1, 0, xs, **param))
                                tmp_yticks = np.delete(tmp_yticks, k)
                            else:
                                k += 1

                    for i in range(1, len(yticks) - 1):
                        y0, y1 = yticks[i:i + 2]
                        k = 1
                        while k < len(xticks) - 1:
                            x0, x1 = xticks[k - 1:k + 1]
                            if abs(self._moebius_z(x0, y1) - self._moebius_z(x1, y1)) < thr_y:
                                self._fancy_majorarcs.append(self.add_realarc(x1, -y0, y0, **param))
                                xticks = np.delete(xticks, k)
                            else:
                                k += 1
                else:
                    draw_standard(xticks, yticks, param, self._fancy_majorarcs)

    def _hack_linedraw(self, line, rotate_marker=None):
        '''
        Modifies the draw method of a :class:`matplotlib.lines.Line2D` object
        to draw different stard and end marker.

        Keyword arguments:

            *line*:
                Line to be modified
                Accepts: Line2D

            *rotate_marker*:
                If set, the end marker will be rotated in direction of their
                corresponding path.
                Accepts: boolean
        '''
        assert isinstance(line, Line2D)

        def new_draw(self_line, renderer):
            def new_draw_markers(self_renderer, gc, marker_path, marker_trans, path, trans, rgbFace=None):
                # get the drawn path for determin the rotation angle
                line_vertices = self_line._get_transformed_path().get_fully_transformed_path().vertices
                vertices = path.vertices

                if len(vertices) == 1:
                    line_set = [[default_marker, vertices]]
                else:
                    if rotate_marker:
                        dx, dy = np.array(line_vertices[-1]) - np.array(line_vertices[-2])
                        end_rot = MarkerStyle(end.get_marker())
                        end_rot._transform += Affine2D().rotate(np.arctan2(dy, dx) - np.pi / 2)
                    else:
                        end_rot = end

                    if len(vertices) == 2:
                        line_set = [[start, vertices[0:1]], [end_rot, vertices[1:2]]]
                    else:
                        line_set = [[start, vertices[0:1]], [default_marker, vertices[1:-1]], [end_rot, vertices[-1:]]]

                for marker, points in line_set:
                    scale = 0.5 if isinstance(marker.get_marker(), np.ndarray) else 1
                    transform = marker.get_transform() + Affine2D().scale(scale * self_line._markersize)
                    old_draw_markers(gc, marker.get_path(), transform, Path(points), trans, rgbFace)

            old_draw_markers = renderer.draw_markers
            renderer.draw_markers = MethodType(new_draw_markers, renderer)
            old_draw(renderer)
            renderer.draw_markers = old_draw_markers

        default_marker = line._marker
        # check if marker is set and visible
        if default_marker:
            start = MarkerStyle(self._get_key("plot.startmarker"))
            if start.get_marker() is None:
                start = default_marker

            end = MarkerStyle(self._get_key("plot.endmarker"))
            if end.get_marker() is None:
                end = default_marker

            if rotate_marker is None:
                rotate_marker = self._get_key("plot.rotatemarker")

            old_draw = line.draw
            line.draw = MethodType(new_draw, line)
            line._markerhacked = True

    def add_realarc(self, xs, y0, y1, **kwargs):
        assert xs >= 0

        return self.add_artist(Line2D(2 * [xs], [y0, y1], **kwargs), path_interpolation='inf_circle')
#         '''
#         Add an arc for a real axis circle.
#
#         Keyword arguments:
#
#             *xs*:
#                 Real axis value
#                 Accepts: float
#
#             *y0*:
#                 Start point xs + y0 * 1j
#                 Accepts: float
#
#             *y1*:
#                 End Poit xs + y1 * 1j
#                 Accepts: float
#
#             **kwargs*:
#                 Keywords passed to the arc creator
#         '''

    def add_imagarc(self, ys, x0, x1, **kwargs):
#         '''
#         Add an arc for a real axis circle.
#
#         Keyword arguments:
#
#             *ys*:
#                 Imaginary axis value
#                 Accepts: float
#
#             *x0*:
#                 Start point x0 + ys * 1j
#                 Accepts: float
#
#             *x1*:
#                 End Poit x1 + ys * 1j
#                 Accepts: float
#
#             **kwargs*:
#                 Keywords passed to the arc creator
#         '''
        assert x0 >= 0 and x1 >= 0

        if abs(ys) > EPSILON:
            steps = 'inf_circle'
        else:
            steps = 1

        return self.add_artist(Line2D([x0, x1], 2 * [ys], **kwargs), path_interpolation=steps)


    def add_artist(self, a, path_interpolation=None):
        '''
        Modified :meth:`matplotlib.axes.Axes.add_artist`. Enables the
        interpolation of a given artist, e.g. a line or rectangle

        Keyword arguments:

            *a*:
                Artist to be added
                Accepts: :class:`matplotlib.artist.Artist` instance

            *path_interpolation`:
                if set, the path of the artist will be interpolated with the
                given value. If set to 0, bezier arcs are used to connect.
        '''
        if path_interpolation is not None:
            if hasattr(a, "get_path"):
                a.get_path()._interpolation_steps = path_interpolation
            else:
                raise AttributeError("Artist has no Path")
        return Axes.add_artist(self, a)


    class MoebiusTransform(Transform):
        '''
        Class for transforming points and paths to Smith Chart data space.
        '''
        input_dims = 2
        output_dims = 2
        is_separable = False
        u = 0

        def __init__(self, axes):
            assert isinstance(axes, SmithAxes)
            Transform.__init__(self)
            self._axes = axes

        def transform_non_affine(self, data):
            def _moebius_xy(xxx_todo_changeme):
                (x, y) = xxx_todo_changeme
                w = self._axes._moebius_z(complex(x, y))
                return [np.real(w), np.imag(w)]

            if isinstance(data[0], Iterable):
                return list(map(_moebius_xy, data))
            else:
                return _moebius_xy(data)

        def transform_path_non_affine(self, path):
            vertices = path.vertices
            codes = path.codes
            steps = path._interpolation_steps

            x, y = np.array(list(zip(*vertices)))

            if len(vertices) > 1 and not isinstance(steps, int):
                if steps == 'inf_circle':
                    z = x + y * 1j
                    new_vertices = []
                    new_codes = []

                    for i in range(len(z) - 1):
                        az = self._axes._moebius_z(0.5 * (z[i] + z[i+1]))
                        zz = self._axes._moebius_z(z[i:i+2])
                        ax, ay = az.real, az.imag
                        bz, cz = zz
                        bx, cx = zz.real - ax
                        by, cy = zz.imag - ay

                        k = 2 * (bx * cy - by * cx)
                        xm = (cy * (bx ** 2 + by ** 2) - by * (cx ** 2 + cy ** 2)) / k + ax
                        ym = (bx * (cx ** 2 + cy ** 2) - cx * (bx ** 2 + by ** 2)) / k + ay

                        zm = xm + ym * 1j
                        d = 2 * abs(zm - az)

                        ang0 = np.angle(bz - zm, deg=True) % 360
                        ang1 = np.angle(cz - zm, deg=True) % 360

                        reverse = ang0 > ang1
                        if reverse:
                            ang0, ang1 = ang1, ang0

                        arc = Arc([xm, ym], d, d, theta1=ang0, theta2=ang1, transform=self._axes.transMoebius)
                        arc_path = arc.get_patch_transform().transform_path(arc.get_path())

                        if reverse:
                            new_vertices.append(arc_path.vertices[::-1])
                        else:
                            new_vertices.append(arc_path.vertices)

                        new_codes.append(arc_path.codes)

                    new_vertices = np.concatenate(new_vertices)
                    new_codes = np.concatenate(new_codes)
                elif steps == 'center_circle':
                    points = self._axes._get_key("path.default_interpolation")

                    z = self._axes._moebius_z(x + y * 1j)

                    ang0, ang1 = np.angle(z[0:2]) % TWO_PI
                    ccw = (ang1 - ang0) % TWO_PI < np.pi

                    ix, iy = [np.real(z[0])], [np.imag(z[0])]
                    new_codes = [Path.MOVETO]

                    for i in range(len(z) - 1):
                        zz = z[i:i + 2]

                        r0, r1 = np.abs(zz)
                        ang0, ang1 = np.angle(zz) % TWO_PI

                        if ccw:
                            if ang0 > ang1:
                                ang1 += TWO_PI
                        else:
                            if ang1 > ang0:
                                ang0 += TWO_PI

                        r = np.linspace(r0, r1, points)[1:]
                        ang = np.linspace(ang0, ang1, points)[1:]

                        ix += list(np.cos(ang) * r)
                        iy += list(np.sin(ang) * r)

                        new_codes += (points - 1) * [Path.LINETO]

                    new_vertices = list(zip(ix, iy))
                else:
                    raise ValueError("Interpolation must be either an integer, 'inf_circle' or 'center_circle'")
            else:
                if steps == 0:
                    steps = self._axes._get_key("path.default_interpolation")

                ix, iy = ([x[0:1]], [y[0:1]])
                for i in range(len(x) - 1):
                    x0, x1 = x[i:i + 2]
                    y0, y1 = y[i:i + 2]


                    tx = self._axes.real_interp1d([x0, x1], steps)[1:]
                    if abs(x0 - x1) > EPSILON:
                        ty = y0 + (tx - x0) * (y1 - y0) / (x1 - x0)
                    else:
                        ty = self._axes.imag_interp1d([y0, y1], steps)[1:]

                    ix.append(tx)
                    iy.append(ty)

                if codes is not None:
                    new_codes = Path.LINETO * np.ones((len(codes) - 1) * steps + 1)
                    new_codes[0::steps] = codes
                else:
                    new_codes = None

                new_vertices = self.transform_non_affine(list(zip(np.concatenate(ix), np.concatenate(iy))))
                new_codes = codes

            return Path(new_vertices, new_codes, 1)

        def inverted(self):
            return SmithAxes.InvertedMoebiusTransform(self._axes)


    class InvertedMoebiusTransform(Transform):
        '''
        Inverse transformation for points and paths in Smith Chart data space.
        '''
        input_dims = 2
        output_dims = 2
        is_separable = False

        def __init__(self, axes):
            assert isinstance(axes, SmithAxes)
            Transform.__init__(self)
            self._axes = axes

        def transform_non_affine(self, xy):
            def _moebius_inv_xy(xxx_todo_changeme1):
                (x, y) = xxx_todo_changeme1
                w = self._axes._moebius_inv_z(complex(x, y))
                return [np.real(w), np.imag(w)]

            return list(map(_moebius_inv_xy, xy))

        def inverted(self):
            return SmithAxes.MoebiusTransform(self._axes)


    class PolarTranslate(Transform):
        '''
        Transformation for translating points away from the center by a given
        padding.

        Keyword arguments:

            *axes*:
                Parent :class:`SmithAxes`
                Accepts: SmithAxes instance

            *pad*:
                Distance to translate away from center for x and y values.

            *font_size*:
                y values are shiftet 0.5 * font_size further away.
        '''
        input_dims = 2
        output_dims = 2
        is_separable = False

        def __init__(self, axes, pad, font_size):
            Transform.__init__(self, shorthand_name=None)
            self.axes = axes
            self.pad = pad
            self.font_size = font_size

        def transform_non_affine(self, xy):
            def translate(xxx_todo_changeme2):
                (x, y) = xxx_todo_changeme2
                ang = np.angle(complex(x - x0, y - y0))
                return [x + np.cos(ang) * (self.pad),
                        y + np.sin(ang) * (self.pad + 0.5 * self.font_size)]

            x0, y0 = self.axes.transAxes.transform([0.5, 0.5])
            if isinstance(xy[0], Iterable):
                return list(map(translate, xy))
            else:
                return translate(xy)


    class RealMaxNLocator(Locator):
        '''
        Locator for the real axis of a SmithAxes. Creates a nicely rounded
        spacing with maximum n values. The transformed center value is
        always included.

        Keyword arguments:

            *axes*:
                Parent SmithAxes
                Accepts: SmithAxes instance

            *n*:
                Maximum number of divisions
                Accepts: integer

            *precision*:
                Maximum number of significant decimals
                Accepts: integer
        '''
        def __init__(self, axes, n, precision=None):
            assert isinstance(axes, SmithAxes)
            assert n > 0

            Locator.__init__(self)
            self.steps = n
            if precision is None:
                self.precision = axes._get_key("grid.locator.precision")
            else:
                self.precision = precision
            assert self.precision > 0

            self.ticks = None
            self.axes = axes

        def __call__(self):
            if self.ticks is None:
                self.ticks = self.tick_values(0, self.axes._inf)
            return self.ticks

        def nice_round(self, num, down=True):
            # normalize to 'precision' decimals befor comma
            exp = np.ceil(np.log10(np.abs(num) + EPSILON))
            if exp < 1:  # fix for leading 0
                exp += 1
            norm = 10 ** -(exp - self.precision)

            num_normed = num * norm
            # increase precision by 0.5, if normed value is smaller than 1/3
            # of its decade range
            if num_normed < 3.3:
                norm *= 2
            # decrease precision by 1, if normed value is bigger than 1/2
            elif num_normed > 50:
                norm /= 10

            # select rounding function
            if not 1 < num_normed % 10 < 9:
                # round to nearest value, if last digit is 1 or 9
                if abs(num_normed % 10 - 1) < EPSILON:
                    num -= 0.5 / norm
                f_round = np.round
            else:
                f_round = np.floor if down else np.ceil

            return f_round(np.round(num * norm, 1)) / norm

        def tick_values(self, vmin, vmax):
            tmin, tmax = self.transform(vmin), self.transform(vmax)
            mean = self.transform(self.nice_round(self.invert(0.5 * (tmin + tmax))))

            result = [tmin, tmax, mean]
            d0 = abs(tmin - tmax) / (self.steps + 1)
            # calculate values above and below mean, adapt delta
            for sgn, side, end in [[1, False, tmax], [-1, True, tmin]]:
                d, d0 = d0, None
                last = mean
                while True:
                    new = last + d * sgn
                    if self.out_of_range(new) or abs(end - new) < d / 2:
                        break

                    # round new value to the next nice display value
                    new = self.transform(self.nice_round(self.invert(new), side))
                    d = abs(new - last)
                    if d0 is None:
                        d0 = d

                    last = new
                    result.append(last)

            return np.sort(self.invert(np.array(result)))

        def out_of_range(self, x):
            return abs(x) > 1

        def transform(self, x):
            return self.axes._moebius_z(x)

        def invert(self, x):
            return self.axes._moebius_inv_z(x)

    class ImagMaxNLocator(RealMaxNLocator):
        def __init__(self, axes, n, precision=None):
            SmithAxes.RealMaxNLocator.__init__(self, axes, n // 2, precision)

        def __call__(self):
            if self.ticks is None:
                tmp = self.tick_values(0, self.axes._inf)
                self.ticks = np.concatenate((-tmp[:0:-1], tmp))
            return self.ticks

        def out_of_range(self, x):
            return not 0 <= x <= np.pi

        def transform(self, x):
            return np.pi - np.angle(self.axes._moebius_z(x * 1j))

        def invert(self, x):
            return np.imag(-self.axes._moebius_inv_z(ang_to_c(np.pi + np.array(x))))

    class SmithAutoMinorLocator(AutoMinorLocator):
        '''
        AutoLocator for SmithAxes. Returns linear spaced intermediate ticks
        depending on the major tickvalues.

        Keyword arguments:

            *n*:
                Number of intermediate ticks
                Accepts: positive integer
        '''
        def __init__(self, n=None):
            assert n > 0
            AutoMinorLocator.__init__(self, n=n)
            if n is None:
                #TODO: is _get_key valid?
                self.ndivs = self._get_key("grid.minor.xauto")
            self._ticks = None

        def __call__(self):
            if self._ticks is None:
                majorticks = self.axis.get_majorticklocs()
                linspace_mod = lambda i0_i1: np.linspace(i0_i1[0], i0_i1[1], self.ndivs + 1)
                intervals = list(zip(majorticks[:-1], majorticks[1:]))
                minorticks = np.concatenate(list(map(linspace_mod, intervals)))
                self._ticks = np.sort(np.concatenate([majorticks, minorticks]))
            return self._ticks

    class RealFormatter(Formatter):
        '''
        Formatter for the real axis of a SmithAxes. Prints the numbers as
        float and removes trailing zeros and commata. Special returns:
            '' for 0.

        Keyword arguments:

            *axes*:
                Parent axes
                Accepts: SmithAxes instance
        '''
        def __init__(self, axes, *args, **kwargs):
            assert isinstance(axes, SmithAxes)
            Formatter.__init__(self, *args, **kwargs)
            self._axes = axes

        def __call__(self, x, pos=None):
            if x < EPSILON or x > self._axes._near_inf:
                return ""
            else:
                return ('%f' % x).rstrip('0').rstrip('.')

    class ImagFormatter(RealFormatter):
        '''
        Formatter for the imaginary axis of a SmithAxes. Prints the numbers
        as  float and removes trailing zeros and commata. Special returns:
            - '' for minus infinity
            - 'symbol.infinity' from scParams for plus infinity
            - '0' for value near zero (prevents -0)

        Keyword arguments:

            *axes*:
                Parent axes
                Accepts: SmithAxes instance
        '''
        def __call__(self, x, pos=None):
            if x < -self._axes._near_inf:
                return ""
            elif x > self._axes._near_inf:
                return self._axes._get_key("symbol.infinity")  # utf8 infinity symbol
            elif abs(x) < EPSILON:
                return "0"
            else:
                return ("%f" % x).rstrip('0').rstrip('.') + "j"

    # update docstrings for all methode not set
    for key, value in locals().copy().items():
        if isinstance(value, FunctionType):
            if value.__doc__ is None and hasattr(Axes, key):
                value.__doc__ = getattr(Axes, key).__doc__

__author__ = "Paul Staerke"
__copyright__ = "Copyright 2013, Paul Staerke"
__license__ = "BSD"
__version__ = "0.1"
__maintainer__ = "Paul Staerke"
__email__ = "paul.staerke@gmail.com"
__status__ = "Prototype"


