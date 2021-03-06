/***********************************************************************************/
/*  Copyright 2013 WSL Institute for Snow and Avalanche Research    SLF-DAVOS      */
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
#ifndef TAUCLDGENERATOR_H
#define TAUCLDGENERATOR_H

#include <meteoio/dataGenerators/GeneratorAlgorithms.h>
#include <meteoio/meteoLaws/Sun.h>

#include <map>

namespace mio {

/**
 * @class TauCLDGenerator
 * @brief Atmospheric transmissivity generator.
 * Generate the atmospheric transmissivity (or clearness index, see \ref meteoparam) from other parameters. If a parameter
 * named "CLD" is available, it will be interpreted as cloud cover / cloudiness: in okta between
 * 0 (fully clear) and 8 (fully cloudy). For synop reports, it is possible to include a value of exactly 9 (sky obstructed
 * from view by fog, heavy precipitation...) that will be transparently reset to 8 (fully cloudy).
 *
 * If no such parameter is available, the atmospheric transmissivity is calculated from the solar index
 * (ratio of measured iswr to potential iswr, therefore using the current location (lat, lon, altitude) and ISWR
 * to parametrize the cloud cover). This relies on (Kasten and Czeplak, 1980).
 *
 * @code
 * TAU_CLD::generators = TAU_CLD
 * @endcode
 */
class TauCLDGenerator : public GeneratorAlgorithm {
	public:
		typedef enum CLF_PARAMETRIZATION {
			KASTEN,
			CLF_CRAWFORD
		} clf_parametrization;

		TauCLDGenerator(const std::vector<std::string>& vecArgs, const std::string& i_algo)
			: GeneratorAlgorithm(vecArgs, i_algo), last_cloudiness() { parse_args(vecArgs); }
		bool generate(const size_t& param, MeteoData& md);
		bool create(const size_t& param, std::vector<MeteoData>& vecMeteo);
		static double getCloudiness(const clf_parametrization& clf_model, const MeteoData& md, SunObject& sun, bool &is_night);
	private:
		std::map< std::string, std::pair<double, double> > last_cloudiness; //as < station_hash, <julian_gmt, cloudiness> >
};

} //end namespace mio

#endif
