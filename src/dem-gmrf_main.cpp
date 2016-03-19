/* +---------------------------------------------------------------------------+
   |                                DEM-GMRF                                   |
   |                    https://github.com/3DLAB-UAL/dem-gmrf                  |
   |                                                                           |
   | Copyright (c) 2016, J.L.Blanco - University of Almeria                    |
   | Released under GNU GPL v3 License. See LICENSE file                       |
   +---------------------------------------------------------------------------+ */

#include <mrpt/maps/CHeightGridMap2D_MRF.h>
#include <mrpt/gui.h>
#include <mrpt/random.h>
#include <mrpt/math/CMatrix.h>
#include <mrpt/otherlibs/tclap/CmdLine.h>
#include <mrpt/system/os.h>
#include <mrpt/system/filesystem.h>
#include <mrpt/utils/CTimeLogger.h>
#include <mrpt/math/utils.h>
#include <mrpt/utils/round.h>
#include <mrpt/utils/CFileOutputStream.h>
#include <mrpt/math/ops_containers.h>
#include <algorithm> // std::random_shuffle
#include <ctime>     // std::time
#include <cstdlib>   // std::rand, std::srand

using namespace mrpt;
using namespace mrpt::maps;
using namespace mrpt::math;
using namespace mrpt::system;
using namespace mrpt::utils;
using namespace mrpt::opengl;
using namespace std;

// Declare the supported options.
TCLAP::CmdLine cmd("dem-gmrf", ' ', mrpt::system::MRPT_getVersion().c_str());

TCLAP::ValueArg<std::string>  arg_in_file("i","input","Input dataset file: X,Y,Z points in plain text format",true,"","xyz.txt",cmd);
TCLAP::ValueArg<double>       arg_dem_resolution("r","resolution","Resolution (side length) of each cell in the DEM (meters)",false,1.0,"1.0",cmd);
TCLAP::ValueArg<std::string>  arg_out_prefix("o","output-prefix","Prefix for all output filenames",false,"demgmrf_out","demgmrf_out",cmd);

TCLAP::ValueArg<double>       arg_checkpoints_ratio("c","checkpoint-ratio",
	"Ratio (1.0=all,0.0=none) of data points to use as checkpoints. "
	"They will not be inserted in the DEM. (Default=0.01)",false,0.01,"0.01",cmd);

int dem_gmrf_main(int argc, char **argv)
{
	if (!cmd.parse( argc, argv )) // Parse arguments:
		return 1; // should exit.

	mrpt::utils::CTimeLogger timlog;

	printf(" dem-gmrf (C) University of Almeria\n");
	printf(" Powered by %s - BUILD DATE %s\n", MRPT_getVersion().c_str(), MRPT_getCompilationDate().c_str());
	printf("-------------------------------------------------------------------\n");

	MRPT_TODO("Tipos de ficheros: 3 cols vs 4 cols; Z: 1e+38 error en raster (preguntar no data)");

	const std::string sDataFile = arg_in_file.getValue();
	ASSERT_FILE_EXISTS_(sDataFile);

	printf("\n[1] Loading `%s`...\n", sDataFile.c_str());
	timlog.enter("1.load_dataset");

	CMatrix raw_xyz;
	raw_xyz.loadFromTextFile(sDataFile.c_str());
	const size_t N = raw_xyz.rows(), nCols = raw_xyz.cols();
	printf("[1] Done. Points: %7u  Columns: %3u\n", (unsigned int)N, (unsigned int)nCols);

	timlog.leave("1.load_dataset");
	ASSERT_(nCols>=3);
	
	// ---------------
	printf("\n[2] Determining bounding box...\n");
	timlog.enter("2.bbox");

	double minx = std::numeric_limits<double>::max();
	double miny = std::numeric_limits<double>::max();
	double minz = std::numeric_limits<double>::max();
	double maxx = -std::numeric_limits<double>::max();
	double maxy = -std::numeric_limits<double>::max();
	double maxz = -std::numeric_limits<double>::max();

	for (size_t i=0;i<N;i++)
	{
		const mrpt::math::TPoint3D pt( raw_xyz(i,0),raw_xyz(i,1),raw_xyz(i,2) );
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

	timlog.leave("2.bbox");
	printf("[2] Bbox: x=%11.2f <-> %11.2f (D=%11.2f)\n", minx,maxx,maxx-minx);
	printf("[2] Bbox: y=%11.2f <-> %11.2f (D=%11.2f)\n", miny,maxy,maxy-miny);
	printf("[2] Bbox: z=%11.2f <-> %11.2f (D=%11.2f)\n", minz,maxz,maxz-minz);


	// ---------------
	printf("\n[3] Picking random checkpoints...\n");
	timlog.enter("3.select_chkpts");

	const double chkpts_ratio = arg_checkpoints_ratio.getValue();
	ASSERT_(chkpts_ratio>=0.0 && chkpts_ratio <=1.0);

	// Generate a list with all indices, then keep the first "N-Nchk" for insertion in the map, "Nchk" as checkpoints
	std::vector<size_t> pts_indices;
	mrpt::math::linspace((size_t)0,N-1,N, pts_indices);

	std::srand (unsigned(std::time(0)));
	std::random_shuffle(pts_indices.begin(), pts_indices.end());
	const size_t N_chk_pts    = mrpt::utils::round( chkpts_ratio * N );
	const size_t N_insert_pts = N - N_chk_pts;

	timlog.leave("3.select_chkpts");
	printf("[3] Checkpoints: %9u (%.02f%%)  Rest of points: %9u\n", (unsigned)N_chk_pts, 100.0*chkpts_ratio, (unsigned)N_insert_pts );
	
	// ---------------
	printf("\n[4] Initializing RMF DEM map estimator...\n");
	timlog.enter("4.dem_map_init");

	const double RESOLUTION = arg_dem_resolution.getValue();

	mrpt::maps::CHeightGridMap2D_MRF  dem_map(
		CRandomFieldGridMap2D::mrGMRF_SD /*map type*/,
		minx,maxx,
		miny,maxy,
		RESOLUTION /* resolution */
		);
	timlog.leave("4.dem_map_init");
	printf("[4] Done.\n");

	dem_map.enableVerbose(true);

	// ---------------
	printf("\n[5] Inserting %u points in DEM map...\n",(unsigned)N_insert_pts);
	timlog.enter("5.dem_map_insert_points");

	for (size_t k=0;k<N_insert_pts;k++)
	{
		const size_t i=pts_indices[k];
		const mrpt::math::TPoint3D pt( raw_xyz(i,0),raw_xyz(i,1),raw_xyz(i,2) );
		
		MRPT_TODO("SIGMAS diferentes!!");

		dem_map.insertIndividualReading(pt.z, TPoint2D(pt.x,pt.y), false /* do not update map now */ );
	}
	timlog.leave("5.dem_map_insert_points");
	printf("[5] Done.\n");

	// ---------------
	printf("\n[6] Running GMRF estimator...\n");
	timlog.enter("6.dem_map_update_gmrf");

	dem_map.updateMapEstimation();

	timlog.leave("6.dem_map_update_gmrf");
	printf("[6] Done.\n");

	MRPT_TODO("Export residuos");
	MRPT_TODO("Save sep file for checkpoints & used data points");

	// ---------------
	printf("\n[9] Generate TXT output files...\n");
	timlog.enter("9.save_points");

	const string sPrefix = arg_out_prefix.getValue();
	
	{
		CFileOutputStream  fil_pts_map( sPrefix + string("_pts_map.txt") );
		for (size_t k=0;k<N_insert_pts;k++)
		{
			const size_t i=pts_indices[k];
			fil_pts_map.printf("%f, %f, %f\n",raw_xyz(i,0),raw_xyz(i,1),raw_xyz(i,2));
		}
	}
	{
		CFileOutputStream  fil_pts_chk( sPrefix + string("_pts_chk.txt") );
		for (size_t k=0;k<N_chk_pts;k++)
		{
			const size_t i=pts_indices[k+N_insert_pts];
			fil_pts_chk.printf("%f, %f, %f\n",raw_xyz(i,0),raw_xyz(i,1),raw_xyz(i,2));
		}
	}

	dem_map.saveMetricMapRepresentationToFile(sPrefix + string("_grmf") );
	dem_map.saveAsMatlab3DGraph(sPrefix + string("_grmf_draw.m") );

	timlog.leave("9.save_points");
	printf("[9] Done.\n");

#if MRPT_HAS_WXWIDGETS
	registerClass( CLASS_ID( CSetOfObjects ) );

	// 3D view:
	mrpt::opengl::CSetOfObjectsPtr glObj_mean = mrpt::opengl::CSetOfObjects::Create();
	//mrpt::opengl::CSetOfObjectsPtr glObj_var  = mrpt::opengl::CSetOfObjects::Create();
	dem_map.getAs3DObject( glObj_mean );//, glObj_var );
	glObj_mean->setLocation( -0.5*(minx+maxx), -0.5*(miny+maxy), -0.5*(minz+maxz) );
	//glObj_var->setLocation( -0.5*(minx+maxx), -0.5*(miny+maxy), -0.5*(minz+maxz) );

	mrpt::gui::CDisplayWindow3D win("Map",640,480);
	win.setCameraZoom(maxz-minz);
	mrpt::opengl::COpenGLScenePtr &scene = win.get3DSceneAndLock();
	scene->insert( glObj_mean );
	//scene->insert( glObj_var );
	win.unlockAccess3DScene();
	win.repaint();

	win.waitForKey();
#endif
	return 0;
}

int main(int argc, char **argv)
{
	try {
		return dem_gmrf_main(argc,argv);
	} catch (exception &e) {
		cerr << "Exception:\n" << e.what() << endl;
		return 1;
	}
}
