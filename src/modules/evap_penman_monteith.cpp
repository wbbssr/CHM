
#include "evap_penman_monteith.h"

evap_penman_monteith::evap_penman_monteith( std::string ID)
{

    _depends->push_back("Qsi");
    _depends->push_back("Lin");
//    _depends->push_back("albdeo");
    _depends->push_back("es");
    _depends->push_back("ea");
    _depends->push_back("t");
    _depends->push_back("u");

    _provides->push_back("ET");


    this->ID = ID;
    _parallel_type = parallel::data;
    LOG_DEBUG << "Successfully instantiated module " << this->ID;
}

 void evap_penman_monteith::run(mesh_elem& elem, boost::shared_ptr<global> global_param)
{

    double albedo = 0.23; //grass and crops
    double qsi = elem->face_data("Qsi");
    double Lin = elem->face_data("Lin");
    double es = elem->face_data("es")/1000.0;
    double ea = elem->face_data("ea")/1000.0;
    double T = elem->face_data("t");
    double u = elem->face_data("u");


    double grass_emissivity = 0.9;


    double Qn = (1-albedo)*qsi;
    double sigma = 5.67*pow(10.0,-8.0); //boltzman
    double Lout = sigma * grass_emissivity * pow(T+273,4.0); //assume ground temp = air temp (lol)

    double Rn = Qn + (Lin-Lout);

    double G = 0.1*Rn;

    double delta = ( 4098.0*(0.6108*exp( (17.27*T) / (T+237.3))))/pow(T+237.3,2.0);

    double psy_const = 0.066; //kpa / K

    double latent_heat = 2501.0-2.361*T; //kJ/kg

    double cp = 1.005; //kJ/kg

    double rho = 1.2; //density dry air, take it as const for now.

    double h = 0.01; //veg height

    double z0 = h/7.6; //maybe fix this?

    double kappa = 0.41;

    double ra = pow(log( (10.0-0.67*h)/z0),2.0)/(pow(kappa,2.0)*u); //10cm veg

    double rc = 62.0; //s/m  unstressed

    double E = (delta*(Qn-G)/latent_heat + (rho*cp*(es-ea)/ra))/(delta + psy_const * (1+rc/ra));

    elem->set_face_data("ET", E);


}

evap_penman_monteith::~evap_penman_monteith()
{

}