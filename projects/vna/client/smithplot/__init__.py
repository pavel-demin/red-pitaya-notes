from matplotlib.projections import register_projection
from smithplot.smithaxes import SmithAxes
import matplotlib

# check version requierment
if matplotlib.__version__ < '1.2':
    raise ImportError("pySmithPlot requires at least matplotlib version 1.2")

# add smith projection to available projections
register_projection(SmithAxes)
