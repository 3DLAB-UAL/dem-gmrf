# dem-gmrf
An application for generating Digital Elevation Models (DEM) with Gaussian Markov Random Fields (GMRF). Works on Windows, GNU/Linux and Mac OS.

**References:**
  * F.J. Aguilar, M. Aguilar, J.L. Blanco, A. Nemmaoui, "Analysis and Validation of Grid DEM Generation Based on Gaussian Markov Random Field (GMRF)", ISPRS Congress, Praga, 2016. ([PDF](http://ingmec.ual.es/~jlblanco/papers/isprs-archives-XLI-B2-277-2016.pdf))

# Installing

*  Windows binaries:
  * (Write me!)
*  Building from sources requires:
  * A C++ compiler: GCC, clang, Visual C++ 2012 or above recommended.
  * [MRPT](www.mrpt.org) >= 1.4.0

# Input file formats

* XYZ: Plain text file, each row containing one height (Z) measurement: (separated by whitespaces or commas `,`)

        X    Y    Z

* XYZS: Plain text file, each row containing one height (Z) measurement and its uncertainty (standard deviation): (separated by whitespaces or commas `,`)

        X    Y    Z  STD_DEV

# Usage

		   dem-gmrf  [--no-gui] [--skip-variance] [--std-obs <0.20>] [--std-prior
					 <1.0>] [-c <0.01>] [-o <demgmrf_out>] [-r <1.0>] -i <xyz.txt>
					 [--] [--version] [-h]


		Where:

		   --no-gui
			 Do not show the graphical window with the 3D visualization at end.

		   --skip-variance
			 Skip variance estimation

		   --std-obs <0.20>
			 Default standard deviation of each XYZ point observation [meters]

		   --std-prior <1.0>
			 Standard deviation of the prior constraints (`smoothness` or
			 `tolerance`of the terrain) [meters]

		   -c <0.01>,  --checkpoint-ratio <0.01>
			 Ratio (1.0=all,0.0=none) of data points to use as checkpoints. They
			 will not be inserted in the DEM. (Default=0.01)

		   -o <demgmrf_out>,  --output-prefix <demgmrf_out>
			 Prefix for all output filenames

		   -r <1.0>,  --resolution <1.0>
			 Resolution (side length) of each cell in the DEM (meters)

		   -i <xyz.txt>,  --input <xyz.txt>
			 (required)  Input dataset file: X,Y,Z points in plain text format

		   --,  --ignore_rest
			 Ignores the rest of the labeled arguments following this flag.

		   --version
			 Displays version information and exits.

		   -h,  --help
			 Displays usage information and exits.
				
				


