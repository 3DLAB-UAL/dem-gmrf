/* +---------------------------------------------------------------------------+
   |                                DEM-GMRF                                   |
   |                    https://github.com/3DLAB-UAL/dem-gmrf                  |
   |                                                                           |
   | Copyright (c) 2016, J.L.Blanco - University of Almeria                    |
   | Released under GNU GPL v3 License. See LICENSE file                       |
   +---------------------------------------------------------------------------+ */

#include <mrpt/maps/CGasConcentrationGridMap2D.h>
#include <mrpt/gui.h>
#include <mrpt/random.h>
#include <mrpt/math/CMatrix.h>
#include <mrpt/otherlibs/tclap/CmdLine.h>
#include <mrpt/system/os.h>

using namespace mrpt;
using namespace mrpt::maps;
using namespace mrpt::math;
using namespace mrpt::random;
using namespace mrpt::system;
using namespace std;

// Declare the supported options.
TCLAP::CmdLine cmd("dem-gmrf", ' ', mrpt::system::MRPT_getVersion().c_str());

//TCLAP::ValueArg<std::string>	arg_in_rawlog("i","input-rawlog","Input dataset",true,"","input.rawlog",cmd);
//TCLAP::ValueArg<unsigned int >	arg_corners_decim("","corners-decimation","KF corners  (to decimate)",false,20,"20",cmd);


int dem_gmrf_main(int argc, char **argv)
{
	if (!cmd.parse( argc, argv )) // Parse arguments:
		return 1; // should exit.

	printf(" dem-gmrf (C) University of Almeria\n");
	printf(" Powered by MRPT %s - BUILD DATE %s\n", MRPT_getVersion().c_str(), MRPT_getCompilationDate().c_str());
	printf("-------------------------------------------------------------------\n");



	MRPT_TODO("Tipos de ficheros: 3 cols vs 4 cols; Z: 1e+38 error en raster (preguntar no data)");

	const std::string sDataFile = "..\\..\\workingarea.txt";

	printf("Loading `%s`...", sDataFile.c_str());

	CMatrix raw_lidar;
	raw_lidar.loadFromTextFile(sDataFile.c_str());

	printf("ok\n");

	const size_t N = raw_lidar.rows(), nCols = raw_lidar.cols();
	ASSERT_(nCols>=3);
	
	double minx = std::numeric_limits<double>::max();
	double miny = std::numeric_limits<double>::max();
	double minz = std::numeric_limits<double>::max();
	double maxx = -std::numeric_limits<double>::max();
	double maxy = -std::numeric_limits<double>::max();
	double maxz = -std::numeric_limits<double>::max();

	for (size_t i=0;i<N;i++)
	{
		const mrpt::math::TPoint3D pt( raw_lidar(i,0),raw_lidar(i,1),raw_lidar(i,2) );
		mrpt::utils::keep_max(maxx,pt.x); mrpt::utils::keep_min(minx,pt.x);
		mrpt::utils::keep_max(maxy,pt.y); mrpt::utils::keep_min(miny,pt.y);
		if (std::abs(pt.z)<1e6) {
			mrpt::utils::keep_max(maxz,pt.z); mrpt::utils::keep_min(minz,pt.z);
		}
	}

	const double BORDER = 10.0;
	minx-= BORDER; maxx += BORDER;
	miny-= BORDER; maxy += BORDER;
	minz-= BORDER; maxz += BORDER;

	printf("Bbox: x=%f <-> %f\n", minx,maxx);
	printf("Bbox: y=%f <-> %f\n", miny,maxy);
	printf("Bbox: z=%f <-> %f\n", minz,maxz);

	printf("Inicializando mapa...");

	const double RESOLUTION = 1.0;
	MRPT_TODO("as a param");

	mrpt::maps::CGasConcentrationGridMap2D  gasmap(
		CRandomFieldGridMap2D::mrGMRF_SD /*map type*/,
		minx,maxx,
		miny,maxy,
		RESOLUTION /* resolution */
		);
	printf("ok\n");

	gasmap.insertionOptions.GMRF_use_occupancy_information = false;

	MRPT_TODO("Select % of checkpoints; insert the rest")

	printf("Insertando observaciones...");
	for (size_t i=0;i<N/10;i++)
	{
		const mrpt::math::TPoint3D pt( raw_lidar(i,0),raw_lidar(i,1),raw_lidar(i,2) );
		MRPT_TODO("SIGMAS diferentes!!");
		gasmap.insertIndividualReading(pt.x, TPoint2D(pt.x,pt.y), false /* do not update map now */ );
	}
	printf("ok\n");

	printf("Update map...");
	gasmap.updateMapEstimation();
	printf("done\n");

	MRPT_TODO("Pedir prefix/sufijo");
	
	MRPT_TODO("Export residuos");
	MRPT_TODO("Save sep file for checkpoints & used data points");

	gasmap.saveToTextFile("grid_mean.txt");

	// 3D view:
	mrpt::opengl::CSetOfObjectsPtr glObj = mrpt::opengl::CSetOfObjects::Create();
	gasmap.getAs3DObject( glObj );

	mrpt::gui::CDisplayWindow3D win("Map",640,480);

	mrpt::opengl::COpenGLScenePtr &scene = win.get3DSceneAndLock();
	scene->insert( glObj );
	win.unlockAccess3DScene();
	win.repaint();

	win.setCameraPointingToPoint( 0.5*(minx+maxx), 0.5*(miny+maxy), 0.5*(minz+maxz) );

	win.waitForKey();
}

int main(int argc, char **argv)
{
	try {
		return dem_gmrf_main(argc,*argv);
	} catch (exception &e) {
		cerr << "Exception:\n" << e.what() << endl;
		return 1;
	}
}
