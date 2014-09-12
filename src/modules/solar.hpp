#pragma once

#include "logger.hpp"
#include "triangulation.hpp"
#include "module_base.hpp"

#include <cstdlib>
#include <string>

#include <cmath>
#include <armadillo>
#define _USE_MATH_DEFINES
#include <math.h>

/**
* \addtogroup modules
* @{
* \class Solar
* \brief Calculates shortwave radiation
*
* Calculates incoming direct-beam shortwave solar radiation, with no correction for transmissivity.
*
* Depends:
* - Terrain shadows "shadowed" [-]
*
* Provides:
* - Solar shortwave "Qsi" [W/m^-1]
* - Angle between surface normal and slope "solar_angle" [rad]
*/
class Solar : public module_base
{
    public:
        Solar(std::string ID);
        ~Solar();
        virtual void run(mesh_elem& elem, boost::shared_ptr<global> global_param);


};

/**
@}
*/