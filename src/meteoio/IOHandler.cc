/***********************************************************************************/
/*  Copyright 2009-2012 WSL Institute for Snow and Avalanche Research  SLF-DAVOS   */
/***********************************************************************************/
/* This file is part of MeteoIO.
    MeteoIO is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    MeteoIO is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with MeteoIO.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <meteoio/IOUtils.h>
#include <meteoio/IOHandler.h>

#define PLUGIN_ALPUG
#define PLUGIN_ARCIO
#define PLUGIN_A3DIO
#define PLUGIN_ARPSIO
#define PLUGIN_GRASSIO
#define PLUGIN_GEOTOPIO
#define PLUGIN_SMETIO
#define PLUGIN_SNIO
#define PLUGIN_PGMIO
/* #undef PLUGIN_IMISIO */
/* #undef PLUGIN_GRIBIO */
/* #undef PLUGIN_PNGIO */
/* #undef PLUGIN_BORMAIO */
/* #undef PLUGIN_COSMOXMLIO */
/* #undef PLUGIN_GSNIO */
/* #undef PLUGIN_NETCDFIO */
/* #undef PLUGIN_PSQLIO */
/* #undef PLUGIN_SASEIO */

#include <meteoio/plugins/ALPUG.h>
#include <meteoio/plugins/ARCIO.h>
#include <meteoio/plugins/A3DIO.h>
#include <meteoio/plugins/ARPSIO.h>
#include <meteoio/plugins/GrassIO.h>
#include <meteoio/plugins/GeotopIO.h>
#include <meteoio/plugins/PGMIO.h>
#include <meteoio/plugins/SMETIO.h>
#include <meteoio/plugins/SNIO.h>

#ifdef PLUGIN_BORMAIO
#include <meteoio/plugins/BormaIO.h>
#endif

#ifdef PLUGIN_CNRMIO
#include <meteoio/plugins/CNRMIO.h>
#endif

#ifdef PLUGIN_COSMOXMLIO
#include <meteoio/plugins/CosmoXMLIO.h>
#endif

#ifdef PLUGIN_IMISIO
#include <meteoio/plugins/ImisIO.h>
#endif

#ifdef PLUGIN_GRIBIO
#include <meteoio/plugins/GRIBIO.h>
#endif

#ifdef PLUGIN_GSNIO
#include <meteoio/plugins/GSNIO.h>
#endif

#ifdef PLUGIN_NETCDFIO
#include <meteoio/plugins/NetCDFIO.h>
#endif

#ifdef PLUGIN_PNGIO
#include <meteoio/plugins/PNGIO.h>
#endif

#ifdef PLUGIN_PSQLIO
#include <meteoio/plugins/PSQLIO.h>
#endif

#ifdef PLUGIN_SASEIO
#include <meteoio/plugins/SASEIO.h>
#endif

using namespace std;

namespace mio {
 /**
 * @page plugins Plugins overview
 * The data access is handled by a system of plugins. They all offer the same interface, meaning that a plugin can transparently be replaced by another one. Since they might rely on third party libraries for accessing the data, they have been created as plugins, that is they are  only compiled if requested when configuring the compilation with cmake.
 * A plugin can therefore fail to run if it has not been compiled.
 *
 * @section available_categories Data sources categories
 * Several data sources categories have been defined that can be provided by a different plugin. Each data source category is defined by a specific key in the configuration file (usually, io.ini):
 * - METEO, for meteorological time series
 * - DEM, for Digital Elevation Maps
 * - LANDUSE, for land cover information
 * - GRID2D, for generic 2D grids (they can contain meteo fields and be recognized as such or arbitrary gridded data)
 * - POI, for a list of Points Of Interest that can be used for providing extra information at some specific location (extracting time series at a few selected points, etc)
 *
 * A plugin is "connected" to a given data source category simply by giving its keyword as value for the data source key:
 * @code
 * METEO = SMET
 * DEM = ARC
 * @endcode
 * Each plugin might have its own specific options, meaning that it might require its own keywords. Please check in each plugin documentation the supported options and keys (see links below).
 * Moreover, a given plugin might only support a given category for read or write (for example, PNG: there is no easy and safe way to interpret a given color as a given numeric value without knowing its color scale, so reading a png has been disabled).
 * Finally, the plugins usually don't implement all these categories (for example, ArcGIS file format only describes 2D grids, so the ARC plugin will only deal with 2D grids), so please check what a given plugin implements before connecting it to a specific data source category.
 *
 * @section available_plugins Available plugins
 * So far the following plugins have been implemented (by keyword for the io.ini key/value config file). Please read the documentation for each plugin in order to know the plugin-specific keywords:
 * <center><table border="1">
 * <tr><th>Plugin keyword</th><th>Provides</th><th>Description</th><th>Extra requirements</th></tr>
 * <tr><td>\subpage alpug "ALPUG"</td><td>meteo</td><td>data files generated by the %ALPUG meteo stations</td><td></td></tr>
 * <tr><td>\subpage a3d "A3D"</td><td>meteo, poi</td><td>original Alpine3D meteo files</td><td></td></tr>
 * <tr><td>\subpage arc "ARC"</td><td>dem, landuse, grid2d</td><td>ESRI/ARC ascii grid files</td><td></td></tr>
 * <tr><td>\subpage arps "ARPS"</td><td>dem, grid2d</td><td>ARPS ascii formatted grids</td><td></td></tr>
 * <tr><td>\subpage borma "BORMA"</td><td>meteo</td><td>Borma xml meteo files</td><td><A HREF="http://libxmlplusplus.sourceforge.net/">libxml++</A></td></tr>
 * <tr><td>\subpage cnrm "CNRM"</td><td>dem, grid2d, meteo</td><td>NetCDF meteorological timeseries following the <A HREF="http://www.cnrm.meteo.fr/?lang=en">CNRM</A> schema</td><td><A HREF="http://www.unidata.ucar.edu/downloads/netcdf/index.jsp">NetCDF-C library</A></td></tr>
 * <tr><td>\subpage cosmoxml "COSMOXML"</td><td>meteo</td><td>MeteoSwiss COSMO's postprocessing XML format</td><td><A HREF="http://xmlsoft.org/">libxml2</A></td></tr>
 * <tr><td>\subpage geotop "GEOTOP"</td><td>meteo</td><td>GeoTop meteo files</td><td></td></tr>
 * <tr><td>\subpage grass "GRASS"</td><td>dem, landuse, grid2d</td><td>Grass grid files</td><td></td></tr>
 * <tr><td>\subpage gribio "GRIB"</td><td>meteo, dem, grid2d</td><td>GRIB meteo grid files</td><td><A HREF="http://www.ecmwf.int/products/data/software/grib_api.html">grib-api</A></td></tr>
 * <tr><td>\subpage gsn "GSN"</td><td>meteo</td><td>connects to the Global Sensor Network web service interface</td><td><A HREF="http://curl.haxx.se/libcurl/">libcurl</A></td></tr>
 * <tr><td>\subpage imis "IMIS"</td><td>meteo</td><td>connects to the IMIS database</td><td><A HREF="http://docs.oracle.com/cd/B12037_01/appdev.101/b10778/introduction.htm">Oracle's OCCI library</A></td></tr>
 * <tr><td>\subpage netcdf "NETCDF"</td><td>dem, grid2d</td><td>NetCDF grids</td><td><A HREF="http://www.unidata.ucar.edu/downloads/netcdf/index.jsp">NetCDF-C library</A></td></tr>
 * <tr><td>\subpage pgmio "PGM"</td><td>dem, grid2d</td><td>PGM grid files</td><td></td></tr>
 * <tr><td>\subpage pngio "PNG"</td><td>dem, grid2d</td><td>PNG grid files</td><td><A HREF="http://www.libpng.org/pub/png/libpng.html">libpng</A></td></tr>
 * <tr><td>\subpage psqlio "PSQL"</td><td>meteo</td><td>connects to PostgreSQL database</td><td><A HREF="http://www.postgresql.org/">PostgreSQL</A>'s libpq</td></tr>
 * <tr><td>\subpage sase "SASE"</td><td>meteo</td><td>connects to the SASE database</td><td><A HREF="https://dev.mysql.com/doc/refman/5.0/en/c-api.html">MySQL's C API</A></td></tr>
 * <tr><td>\subpage smetio "SMET"</td><td>meteo, poi</td><td>SMET data files</td><td></td></tr>
 * <tr><td>\subpage snowpack "SNOWPACK"</td><td>meteo</td><td>original SNOWPACK meteo files</td><td></td></tr>
 * </table></center>
 *
 * @section data_manipulations Data generators & exclusion
 * @subsection data_generators Data generators
 * It is also possible to duplicate a meteorological parameter as another meteorological parameter. This is done by specifying a COPY key, following the syntax
 * new_name::COPY = existing_parameter. For example:
 * @code
 * VW_avg::COPY = VW
 * @endcode
 * This creates a new parameter VW_avg that starts as an exact copy of the raw data of VW, for each station. This newly created parameter is
 * then processed as any other meteorological parameter (thus going through filtering, generic processing, spatial interpolations). This only current
 * limitation is that the parameter providing the raw data must be defined for all stations (even if filled with nodata, this is good enough).
 *
 * @subsection data_exclusion Data exclusion
 * It is possible to exclude specific parameters from given stations (on a per station basis). This is either done by using the station ID
 * followed by "::exclude" as key with a space delimited list of \ref meteoparam "meteorological parameters" to exclude for the station as key.
 * Another possibility is to provide a file containing one station ID per line followed by a space delimited list of \ref meteoparam "meteorological parameters"
 * to exclude for the station (the path to the file can be a relative path and will be properly resolved).
 * 
 * The exact opposite can also be done, excluding ALL parameters except the ones declared with the "::keep" statement (or a file containing one station ID
 * per line followed by a space delimited list of \ref meteoparam "meteorological parameters" to keep for the station).
 *
 * @code
 * WFJ2::EXCLUDE = HS PSUM                       ;inline declaration of parameters exclusion
 * KLO3::KEEP = TA RH VW DW                     ;inline declaration of parameters to keep
 *
 * EXCLUDE_FILE = ../input/meteo/excludes.csv    ;parameters exclusions defined in a separate file
 * KEEP_FILE = ../input/meteo/keeps.csv          ;parameters to keep defined in a separate file
 * @endcode
 *
 * In the second example (relying on a separate file), the file "../input/meteo/excludes.csv" could look like this:
 * @code
 * WFJ2 TA RH
 * KLO3 HS PSUM
 * @endcode
 *
 */

IOInterface* IOHandler::getPlugin(const std::string& plugin_name) const
{
#ifdef PLUGIN_ALPUG
	if (plugin_name == "ALPUG") return new ALPUG(cfg);
#endif
#ifdef PLUGIN_ARCIO
	if (plugin_name == "ARC") return new ARCIO(cfg);
#endif
#ifdef PLUGIN_A3DIO
	if (plugin_name == "A3D") return new A3DIO(cfg);
#endif
#ifdef PLUGIN_ARPSIO
	if (plugin_name == "ARPS") return new ARPSIO(cfg);
#endif
#ifdef PLUGIN_GRASSIO
	if (plugin_name == "GRASS") return new GrassIO(cfg);
#endif
#ifdef PLUGIN_GEOTOPIO
	if (plugin_name == "GEOTOP") return new GeotopIO(cfg);
#endif
#ifdef PLUGIN_SMETIO
	if (plugin_name == "SMET") return new SMETIO(cfg);
#endif
#ifdef PLUGIN_SNIO
	if (plugin_name == "SNOWPACK") return new SNIO(cfg);
#endif
#ifdef PLUGIN_PGMIO
	if (plugin_name == "PGM") return new PGMIO(cfg);
#endif
#ifdef PLUGIN_IMISIO
	if (plugin_name == "IMIS") return new ImisIO(cfg);
#endif
#ifdef PLUGIN_GRIBIO
	if (plugin_name == "GRIB") return new GRIBIO(cfg);
#endif
#ifdef PLUGIN_PNGIO
	if (plugin_name == "PNG") return new PNGIO(cfg);
#endif
#ifdef PLUGIN_BORMAIO
	if (plugin_name == "BORMA") return new BormaIO(cfg);
#endif
#ifdef PLUGIN_COSMOXMLIO
	if (plugin_name == "COSMOXML") return new CosmoXMLIO(cfg);
#endif
#ifdef PLUGIN_CNRMIO
	if (plugin_name == "CNRM") return new CNRMIO(cfg);
#endif
#ifdef PLUGIN_GSNIO
	if (plugin_name == "GSN") return new GSNIO(cfg);
#endif
#ifdef PLUGIN_NETCDFIO
	if (plugin_name == "NETCDF") return new NetCDFIO(cfg);
#endif
#ifdef PLUGIN_PSQLIO
	if (plugin_name == "PSQL") return new PSQLIO(cfg);
#endif
#ifdef PLUGIN_SASEIO
	if (plugin_name == "SASE") return new SASEIO(cfg);
#endif

	return NULL; //no plugin found
}

//this is actually an object factory
IOInterface* IOHandler::getPlugin(const std::string& cfgkey, const std::string& cfgsection)
{
	std::string op_src;
	cfg.getValue(cfgkey, cfgsection, op_src);

	if (mapPlugins.find(op_src) == mapPlugins.end()) {
		IOInterface *ioPtr = getPlugin(op_src);
		if (ioPtr==NULL)
			throw IOException("Cannot find plugin " + op_src + " as requested in file " + cfg.getSourceName() + ". Has it been activated through ccmake? Is it declared in IOHandler::getPlugin?", AT);
		else
			mapPlugins[op_src] = ioPtr;
	}

	return mapPlugins[op_src];
}

//Copy constructor
IOHandler::IOHandler(const IOHandler& aio)
           : IOInterface